#pragma once
#include "Workers.hpp"
#include <memory>
#include <functional>
class CameraScheduler {
    int cameraId_;
    ControlWorker control_;
    ReceiveWorker receive_;
public:
    CameraScheduler(int id, std::unique_ptr<CameraModelStrategy> model, std::function<void(Event)> emit)
        : cameraId_(id),control_(id,std::move(model),emit),receive_(id,emit) {}
    void dispatch(Action a) {
        if (a.kind==Action::Kind::StartReceive || a.kind==Action::Kind::StopReceive) receive_.post(a);
        else control_.post(a);
    }
};
