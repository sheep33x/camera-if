#include "Coordinator.hpp"
#include <iostream>
#include <type_traits>
void Coordinator::addCamera(int id, std::unique_ptr<CameraModelStrategy> model) {
    cameras_[id]=std::make_unique<CameraScheduler>(id,std::move(model),[this](Event e){postEvent(e);});
}
void Coordinator::runOnce() {
    auto message=inbox_.pop();
    std::visit([this](auto&& m){
        using T=std::decay_t<decltype(m)>;
        if constexpr (std::is_same_v<T,Request>) {
            auto camera=cameras_.find(m.cameraId);
            if (camera==cameras_.end()) return;
            // Cは進行中のBを中断して受信停止を優先する。
            if (m.command==Command::C) {
                auto active=activeReceiveB_.find(m.cameraId);
                if (active!=activeReceiveB_.end()) {
                    auto old=handlers_.find(active->second);
                    if (old!=handlers_.end()) { old->second->cancel(); handlers_.erase(old); }
                    activeReceiveB_.erase(active);
                }
            }
            // 簡略化: A/B同時実行の厳密な直列化は省略。製品版ではカメラ別待ち行列を追加。
            auto dispatch=[this](Action a){ cameras_.at(a.cameraId)->dispatch(a); };
            auto respond=[](const Request& r,const std::string& result){
                std::cout << "[Response route=" << r.responseRoute << " request=" << r.id << "] " << result << "\n";
            };
            auto h=makeHandler(m,dispatch,respond);
            if (m.command==Command::B) activeReceiveB_[m.cameraId]=m.id;
            handlers_[m.id]=std::move(h);
            handlers_[m.id]->start();
        } else if constexpr (std::is_same_v<T,Event>) {
            auto it=handlers_.find(m.requestId);
            if (it==handlers_.end()) return; // 中断済み/遅延イベントは破棄
            it->second->onEvent(m);
            if (it->second->finished()) {
                auto active=activeReceiveB_.find(m.cameraId);
                if (active!=activeReceiveB_.end() && active->second==m.requestId) activeReceiveB_.erase(active);
                handlers_.erase(it);
            }
        } else if constexpr (std::is_same_v<T,ImageNotification>) {
            auto it=activeReceiveB_.find(m.cameraId);
            if (it!=activeReceiveB_.end()) postEvent({it->second,m.cameraId,EventType::ImageArrived});
        }
    },message);
}
