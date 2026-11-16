#include "CameraActor.hpp"
#include "Workers.hpp"
void CameraActor::finish(const std::string& result) {
    auto request = procedure_->request;
    procedure_.reset();
    respond_(request, result);
    startNext();
}
void CameraActor::onRequest(Request request) {
    if (request.command == Command::C && procedure_ && procedure_->request.command == Command::B) {
        // Already posted operations remain ordered in their worker queues.
        // Only the stop acknowledgement permits the next procedure to start.
        respond_(procedure_->request, "cancelled");
        procedure_ = Procedure{Procedure::Stage::Stop, std::move(request)};
        stream_.post({Action::Kind::StopReceive, procedure_->request.id, id_});
        return;
    }
    pending_.push_back(std::move(request));
    startNext();
}
void CameraActor::startNext() {
    if (procedure_ || pending_.empty()) return;
    auto request = std::move(pending_.front()); pending_.pop_front();
    procedure_ = Procedure{Procedure::Stage::System, std::move(request)};
    auto& r = procedure_->request;
    switch (r.command) {
    case Command::A: case Command::B:
        system_.post({Action::Kind::PrepareSystem,r.id,id_}); break;
    case Command::C:
        procedure_->stage = Procedure::Stage::Stop;
        stream_.post({Action::Kind::StopReceive,r.id,id_}); break;
    case Command::D:
        if (r.writeOperation == WriteOperation::Start) {
            if (!r.sessionId) { finish("D requires sessionId"); return; }
            if (session_ && session_->state == WriteState::Writing) { finish("D session busy"); return; }
            session_ = WriteSession{r.sessionId,WriteState::None};
            procedure_->stage = Procedure::Stage::Write;
            device_.post({Action::Kind::StartWrite,r.id,id_});
        } else {
            if (!session_ || session_->id != r.sessionId) { finish("D unknown session"); return; }
            procedure_->stage = Procedure::Stage::Query;
            device_.post({Action::Kind::QueryWrite,r.id,id_});
        }
        break;
    }
}
void CameraActor::onEvent(const Event& e) {
    if (e.cameraId != id_ || !procedure_ || procedure_->request.id != e.requestId) return;
    auto& p = *procedure_;
    using Stage = Procedure::Stage;
    if (e.type == EventType::Failed) {
        if (p.request.command == Command::D && session_) session_->state = WriteState::Error;
        finish("failed"); return;
    }
    if (p.stage == Stage::System && e.type == EventType::SystemReady) {
        p.stage = Stage::Config; device_.post({Action::Kind::ApplyConfig,e.requestId,id_});
    } else if (p.stage == Stage::Config && e.type == EventType::ControlDone) {
        if (p.request.command == Command::A) { finish("A completed"); return; }
        p.stage = Stage::Capture; device_.post({Action::Kind::StartCapture,e.requestId,id_});
    } else if (p.stage == Stage::Capture && e.type == EventType::CaptureStarted) {
        p.stage = Stage::Receive; stream_.post({Action::Kind::StartReceive,e.requestId,id_});
    } else if (p.stage == Stage::Receive && e.type == EventType::ReceiveStarted) {
        receiving_ = true; p.stage = Stage::Image;
    } else if (p.stage == Stage::Image && e.type == EventType::ImageArrived) {
        // B includes cleanup; a following B cannot overlap a previous receive.
        p.stage = Stage::Stop; stream_.post({Action::Kind::StopReceive,e.requestId,id_});
    } else if (p.stage == Stage::Stop && e.type == EventType::ReceiveStopped) {
        receiving_ = false;
        finish(p.request.command == Command::B ? "B image received" : "C receive stopped");
    } else if (p.stage == Stage::Write && e.type == EventType::WriteAccepted) {
        session_->state = WriteState::Writing; finish("D accepted (writing continues)");
    } else if (p.stage == Stage::Query && e.type == EventType::WriteStatus) {
        session_->state = e.writeState;
        finish(e.writeState == WriteState::Complete ? "D complete" : e.writeState == WriteState::Writing ? "D writing" : "D error/none");
    }
}
