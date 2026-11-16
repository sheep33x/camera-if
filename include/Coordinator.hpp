#pragma once
#include "Workers.hpp"
#include "CameraActor.hpp"
#include <map>
#include <variant>
using CoordinatorMessage = std::variant<Request,Event>;
class Coordinator {
    // Inbox outlives workers, which may emit while joining during destruction.
    ThreadQueue<CoordinatorMessage> inbox_;
    Respond respond_;
    std::map<int,std::unique_ptr<CameraActor>> actors_;
    SystemWorker system_;
    struct CameraWorkers {
        DeviceWorker device;
        StreamWorker stream;
        CameraWorkers(int id, std::unique_ptr<CameraModelStrategy> model, std::function<void(Event)> emit)
            : device(id,std::move(model),emit), stream(id,emit) {}
    };
    std::map<int,std::unique_ptr<CameraWorkers>> workers_;
public:
    explicit Coordinator(Respond respond = {});
    // addCamera and runOnce are called on one coordinator thread; post* are thread safe.
    void addCamera(int id, std::unique_ptr<CameraModelStrategy> model);
    void postRequest(Request r) { inbox_.push(std::move(r)); }
    void postEvent(Event e) { inbox_.push(e); }
    bool runOnce();
    void stop() { inbox_.close(); }
};
