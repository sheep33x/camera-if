#include "CommandHandlers.hpp"
#include <utility>
class AHandler : public ICommandHandler {
public: using ICommandHandler::ICommandHandler;
    void start() override { dispatch_({Action::Kind::ApplyConfig,req_.id,req_.cameraId}); }
    void onEvent(const Event& e) override { if (e.type==EventType::ControlDone) send("A completed"); }
};
class BHandler : public ICommandHandler {
    enum class Stage { Config, Capture, Receive, Image } stage_=Stage::Config;
public: using ICommandHandler::ICommandHandler;
    void start() override { dispatch_({Action::Kind::ApplyConfig,req_.id,req_.cameraId}); }
    void onEvent(const Event& e) override {
        if (stage_==Stage::Config && e.type==EventType::ControlDone) { stage_=Stage::Capture; dispatch_({Action::Kind::StartCapture,req_.id,req_.cameraId}); }
        else if (stage_==Stage::Capture && e.type==EventType::CaptureStarted) { stage_=Stage::Receive; dispatch_({Action::Kind::StartReceive,req_.id,req_.cameraId}); }
        else if (stage_==Stage::Receive && e.type==EventType::ReceiveStarted) { stage_=Stage::Image; }
        else if (stage_==Stage::Image && e.type==EventType::ImageArrived) { send("B image received"); }
    }
};
class CHandler : public ICommandHandler {
public: using ICommandHandler::ICommandHandler;
    void start() override { dispatch_({Action::Kind::StopReceive,req_.id,req_.cameraId}); }
    void onEvent(const Event& e) override { if (e.type==EventType::ReceiveStopped) send("C receive stopped"); }
};
class DHandler : public ICommandHandler {
    bool query_;
public:
    DHandler(Request r, Dispatch d, Respond out, bool query) : ICommandHandler(std::move(r),std::move(d),std::move(out)),query_(query) {}
    void start() override { dispatch_({query_?Action::Kind::QueryWrite:Action::Kind::StartWrite,req_.id,req_.cameraId}); }
    void onEvent(const Event& e) override {
        if (!query_ && e.type==EventType::WriteAccepted) send("D accepted (writing continues)");
        if (query_ && e.type==EventType::WriteStatus) send(e.writeState==WriteState::Writing?"D writing":e.writeState==WriteState::Complete?"D complete":"D error/none");
    }
};
// Dは実際には要求パラメータ（開始/照会）で分岐する。サンプルではIDの偶奇で代用。
std::unique_ptr<ICommandHandler> makeHandler(Request r, Dispatch d, Respond out) {
    switch (r.command) {
    case Command::A: return std::make_unique<AHandler>(r,d,out);
    case Command::B: return std::make_unique<BHandler>(r,d,out);
    case Command::C: return std::make_unique<CHandler>(r,d,out);
    case Command::D: return std::make_unique<DHandler>(r,d,out,r.id%2==0);
    }
    return {};
}
