#pragma once
#include "CameraScheduler.hpp"
#include "CommandHandlers.hpp"
#include "ThreadQueue.hpp"
#include <map>
#include <memory>
#include <variant>
struct ImageNotification { int cameraId; }; // SDK callback -> coordinator に変換する想定
using CoordinatorMessage = std::variant<Request,Event,ImageNotification>;
class Coordinator {
    ThreadQueue<CoordinatorMessage> inbox_;
    std::map<int,std::unique_ptr<CameraScheduler>> cameras_;
    std::map<uint64_t,std::unique_ptr<ICommandHandler>> handlers_;
    std::map<int,uint64_t> activeReceiveB_;
public:
    void addCamera(int id, std::unique_ptr<CameraModelStrategy> model);
    void postRequest(Request r) { inbox_.push(r); }
    void postEvent(Event e) { inbox_.push(e); }
    void postImage(int cameraId) { inbox_.push(ImageNotification{cameraId}); }
    void runOnce(); // デモではmainから呼ぶ。実際は専用ループで回す。
};
