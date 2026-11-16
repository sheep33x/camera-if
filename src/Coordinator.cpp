#include "Coordinator.hpp"
#include <iostream>
#include <stdexcept>
#include <type_traits>
#include <utility>
Coordinator::Coordinator(Respond respond)
    : respond_(std::move(respond)), system_([this](Event e){ postEvent(e); }) {
    if (!respond_) respond_ = [](const Request& r, const std::string& result) {
        std::cout << "[Response route=" << r.responseRoute << " request=" << r.id << "] " << result << '\n';
    };
}
void Coordinator::addCamera(int id, std::unique_ptr<CameraModelStrategy> model) {
    if (!model || actors_.count(id)) throw std::invalid_argument("invalid or duplicate camera");
    auto workers = std::make_unique<CameraWorkers>(id,std::move(model),[this](Event e){ postEvent(e); });
    auto actor = std::make_unique<CameraActor>(
        id, system_, workers->device, workers->stream, respond_);
    workers_.emplace(id,std::move(workers));
    actors_.emplace(id,std::move(actor));
}
bool Coordinator::runOnce() {
    auto message = inbox_.pop();
    if (!message) return false;
    std::visit([this](const auto& m) {
        auto actor = actors_.find(m.cameraId);
        if (actor != actors_.end()) {
            using T = std::decay_t<decltype(m)>;
            if constexpr (std::is_same_v<T,Request>) actor->second->onRequest(m);
            else actor->second->onEvent(m);
        } else {
            using T = std::decay_t<decltype(m)>;
            if constexpr (std::is_same_v<T,Request>) respond_(m,"unknown camera");
        }
    },*message);
    return true;
}
