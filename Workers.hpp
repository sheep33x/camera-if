#pragma once
#include "Types.hpp"
#include "ThreadQueue.hpp"
#include "CameraModel.hpp"
#include <memory>
#include <thread>
#include <functional>
class ControlWorker {
    ThreadQueue<Action> tasks_;
    std::thread thread_;
    std::unique_ptr<CameraModelStrategy> model_;
    std::function<void(Event)> emit_;
    void run(int cameraId);
public:
    ControlWorker(int cameraId, std::unique_ptr<CameraModelStrategy> model, std::function<void(Event)> emit);
    ~ControlWorker();
    void post(Action a) { tasks_.push(a); }
};
class ReceiveWorker {
    ThreadQueue<Action> tasks_;
    std::thread thread_;
    std::function<void(Event)> emit_;
    void run(int cameraId);
public:
    ReceiveWorker(int cameraId, std::function<void(Event)> emit);
    ~ReceiveWorker();
    void post(Action a) { tasks_.push(a); }
};
