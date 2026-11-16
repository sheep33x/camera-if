#pragma once
#include "CameraModel.hpp"
#include "ThreadQueue.hpp"
#include "Types.hpp"
#include <functional>
#include <memory>
#include <thread>
// Members are initialized before the thread starts; destruction always joins.
class SystemWorker {
    ThreadQueue<Action> tasks_;
    std::function<void(Event)> emit_;
    std::thread thread_;
public:
    explicit SystemWorker(std::function<void(Event)> emit);
    ~SystemWorker();
    void post(Action a) { tasks_.push(a); }
};
class DeviceWorker {
    ThreadQueue<Action> tasks_;
    std::unique_ptr<CameraModelStrategy> model_;
    std::function<void(Event)> emit_;
    std::thread thread_;
public:
    DeviceWorker(int id, std::unique_ptr<CameraModelStrategy> model, std::function<void(Event)> emit);
    ~DeviceWorker();
    void post(Action a) { tasks_.push(a); }
};
class StreamWorker {
    ThreadQueue<Action> tasks_;
    std::function<void(Event)> emit_;
    std::thread thread_;
public:
    StreamWorker(int id, std::function<void(Event)> emit);
    ~StreamWorker();
    void post(Action a) { tasks_.push(a); }
};
