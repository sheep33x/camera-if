#include "Workers.hpp"
#include "DummySdk.hpp"
#include <utility>
SystemWorker::SystemWorker(std::function<void(Event)> emit) : emit_(std::move(emit)), thread_([this] {
    SystemSdkHandle sdk;
    while (auto a = tasks_.pop()) {
        sdk.perform("SDK.PrepareSystem()");
        emit_({a->requestId, a->cameraId, EventType::SystemReady});
    }
}) {}
SystemWorker::~SystemWorker() { tasks_.close(); thread_.join(); }
DeviceWorker::DeviceWorker(int id, std::unique_ptr<CameraModelStrategy> model, std::function<void(Event)> emit)
    : model_(std::move(model)), emit_(std::move(emit)), thread_([this,id] {
    ControlSdkHandle sdk(id);
    while (auto a = tasks_.pop()) {
        EventType result;
        switch (a->kind) {
        case Action::Kind::ApplyConfig: sdk.perform(model_->applyConfigRecipe()); result=EventType::ControlDone; break;
        case Action::Kind::StartCapture: sdk.perform(model_->captureRecipe()); result=EventType::CaptureStarted; break;
        case Action::Kind::StartWrite: sdk.perform("SDK.BeginMemoryWrite()"); result=EventType::WriteAccepted; break;
        case Action::Kind::QueryWrite: sdk.perform("SDK.QueryMemoryWrite()"); result=EventType::WriteStatus; break;
        default: continue;
        }
        // Dummy SDK reports completion on the next query. Business state lives in the actor.
        emit_({a->requestId,id,result,result==EventType::WriteStatus ? WriteState::Complete : WriteState::None});
    }
}) {}
DeviceWorker::~DeviceWorker() { tasks_.close(); thread_.join(); }
StreamWorker::StreamWorker(int id, std::function<void(Event)> emit)
    : emit_(std::move(emit)), thread_([this,id] {
    ReceiveSdkHandle sdk(id);
    while (auto a = tasks_.pop()) {
        if (a->kind == Action::Kind::StartReceive) {
            sdk.perform("SDK.BeginReceive(nonblocking)");
            emit_({a->requestId,id,EventType::ReceiveStarted});
            emit_({a->requestId,id,EventType::ImageArrived});
        } else if (a->kind == Action::Kind::StopReceive) {
            sdk.perform("SDK.StopReceive()");
            emit_({a->requestId,id,EventType::ReceiveStopped});
        }
    }
}) {}
StreamWorker::~StreamWorker() { tasks_.close(); thread_.join(); }
