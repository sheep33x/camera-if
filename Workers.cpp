#include "Workers.hpp"
#include "DummySdk.hpp"
#include <iostream>
#include <utility>
// 簡略化: 終了処理は省略し、デモの実行期間中はワーカーが生存する想定。
ControlWorker::ControlWorker(int id, std::unique_ptr<CameraModelStrategy> model, std::function<void(Event)> emit)
    : model_(std::move(model)), emit_(std::move(emit)), thread_([this,id]{run(id);}) {}
ControlWorker::~ControlWorker() { if (thread_.joinable()) thread_.detach(); /* 実製品では停止通知+join必須 */ }
void ControlWorker::run(int cameraId) {
    ControlSdkHandle sdk(cameraId); // 制御SDKハンドルはこのスレッドで生成・使用
    WriteState write = WriteState::None;
    for (;;) {
        Action a = tasks_.pop();
        switch (a.kind) {
        case Action::Kind::ApplyConfig: sdk.perform(model_->applyConfigRecipe()); emit_({a.requestId,cameraId,EventType::ControlDone}); break;
        case Action::Kind::StartCapture: sdk.perform(model_->captureRecipe()); emit_({a.requestId,cameraId,EventType::CaptureStarted}); break;
        case Action::Kind::StartWrite: sdk.perform("SDK.BeginMemoryWrite()"); write=WriteState::Writing; emit_({a.requestId,cameraId,EventType::WriteAccepted,write}); break;
        case Action::Kind::QueryWrite: sdk.perform("SDK.QueryMemoryWrite()"); emit_({a.requestId,cameraId,EventType::WriteStatus,write}); break;
        default: break;
        }
    }
}
ReceiveWorker::ReceiveWorker(int id, std::function<void(Event)> emit) : emit_(std::move(emit)), thread_([this,id]{run(id);}) {}
ReceiveWorker::~ReceiveWorker() { if (thread_.joinable()) thread_.detach(); /* 実製品では停止通知+join必須 */ }
void ReceiveWorker::run(int cameraId) {
    ReceiveSdkHandle sdk(cameraId); // 受信SDKハンドルはこのスレッドで生成・使用
    uint64_t activeB=0;
    for (;;) {
        Action a=tasks_.pop();
        if (a.kind==Action::Kind::StartReceive) {
            activeB=a.requestId;
            sdk.perform("SDK.BeginReceive(nonblocking/callback)");
            emit_({a.requestId,cameraId,EventType::ReceiveStarted});
            // 画像到着は別のSDKコールバック等からイベント化する想定。
        } else if (a.kind==Action::Kind::StopReceive) {
            sdk.perform("SDK.StopReceive()");
            activeB=0;
            emit_({a.requestId,cameraId,EventType::ReceiveStopped});
        }
    }
}
