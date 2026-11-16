#include "Coordinator.hpp"
#include <stdexcept>
#include <vector>
static void check(bool ok) { if (!ok) throw std::runtime_error("check failed"); }
struct Harness {
    std::vector<Action> system, device, stream;
    std::vector<std::pair<uint64_t,std::string>> responses;
    CameraActor actor{1,{
        [this](Action a){ system.push_back(a); },
        [this](Action a){ device.push_back(a); },
        [this](Action a){ stream.push_back(a); },
        [this](const Request& r,const std::string& s){ responses.emplace_back(r.id,s); }}};
    void event(uint64_t id, EventType type, WriteState state=WriteState::None) { actor.onEvent({id,1,type,state}); }
};
int main() {
    {
        Harness h;
        h.actor.onRequest({1,1,Command::B,"a"});
        h.event(1,EventType::SystemReady); h.event(1,EventType::ControlDone);
        h.event(1,EventType::CaptureStarted); h.event(1,EventType::ReceiveStarted);
        h.actor.onRequest({2,1,Command::B,"a"});
        h.event(1,EventType::ImageArrived);
        check(h.responses.empty() && h.system.size()==1);
        h.event(1,EventType::ReceiveStopped);
        check(h.responses.at(0).second=="B image received" && h.system.size()==2);
    }
    {
        Harness h;
        h.actor.onRequest({1,1,Command::B,"a"});
        h.event(1,EventType::SystemReady); h.event(1,EventType::ControlDone);
        h.event(1,EventType::CaptureStarted);
        h.actor.onRequest({2,1,Command::A,"b"});
        h.actor.onRequest({3,1,Command::C,"a"});
        check(h.responses.at(0).second=="cancelled");
        check(h.stream.back().kind==Action::Kind::StopReceive);
        h.event(1,EventType::ReceiveStarted); h.event(1,EventType::ImageArrived);
        h.actor.onEvent({3,2,EventType::ReceiveStopped});
        check(h.responses.size()==1 && h.system.size()==1);
        h.event(3,EventType::ReceiveStopped);
        check(h.responses.at(1).first==3 && h.system.size()==2);
        h.event(2,EventType::SystemReady); h.event(2,EventType::ControlDone);
        check(h.responses.at(2).first==2);
    }
    {
        Harness h;
        h.actor.onRequest({2,1,Command::D,"a",WriteOperation::Start,42});
        h.event(2,EventType::WriteAccepted);
        check(h.responses.at(0).second=="D accepted (writing continues)");
        h.actor.onRequest({3,1,Command::D,"b",WriteOperation::Query,99});
        check(h.responses.back().second=="D unknown session");
        h.actor.onRequest({4,1,Command::D,"a",WriteOperation::Start,43});
        check(h.responses.back().second=="D session busy");
        h.actor.onRequest({5,1,Command::D,"b",WriteOperation::Query,42});
        h.event(5,EventType::WriteStatus,WriteState::Complete);
        check(h.responses.back().second=="D complete");
    }
    {
        Harness h;
        h.actor.onRequest({1,1,Command::A,"a"});
        h.event(1,EventType::Failed);
        check(h.responses.back().second=="failed");
    }
    {
        int count=0;
        Coordinator c([&](const Request&, const std::string&){ ++count; });
        c.addCamera(1,std::make_unique<ModelAStrategy>());
        c.addCamera(2,std::make_unique<ModelBStrategy>());
        c.postRequest({1,1,Command::B,"a"});
        c.postRequest({2,2,Command::D,"b",WriteOperation::Start,7});
        c.postRequest({3,2,Command::D,"c",WriteOperation::Query,7});
        c.postRequest({4,99,Command::A,"a"});
        while (count<4) check(c.runOnce());
        c.stop(); check(!c.runOnce());
        // Destructor joins all five workers before the inbox is destroyed.
    }
    {
        Coordinator c;
        c.addCamera(1,std::make_unique<ModelAStrategy>());
        c.postRequest({1,1,Command::B,"a"});
        c.runOnce(); // Destruction also works while worker activity is pending.
    }
}
