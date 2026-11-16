#include "Coordinator.hpp"
#include <stdexcept>
#include <vector>

static void check(bool ok) {
    if (!ok) throw std::runtime_error("check failed");
}

struct Harness {
    // Workers may emit until their destructors finish joining.
    ThreadQueue<Event> events;
    std::vector<std::pair<uint64_t, std::string>> responses;
    SystemWorker system{[this](Event event) { events.push(event); }};
    DeviceWorker device{1, std::make_unique<ModelAStrategy>(),
                        [this](Event event) { events.push(event); }};
    StreamWorker stream{1, [this](Event event) { events.push(event); }};
    CameraActor actor{1, system, device, stream,
                      [this](const Request& request, const std::string& result) {
                          responses.emplace_back(request.id, result);
                      }};

    Event take(uint64_t requestId, EventType type) {
        auto event = events.pop();
        check(event.has_value());
        check(event->requestId == requestId);
        check(event->cameraId == 1);
        check(event->type == type);
        return *event;
    }

    void deliver(uint64_t requestId, EventType type) {
        actor.onEvent(take(requestId, type));
    }
};

int main() {
    {
        Harness h;
        h.actor.onRequest({1, 1, Command::B, "a"});
        h.deliver(1, EventType::SystemReady);
        h.deliver(1, EventType::ControlDone);
        h.deliver(1, EventType::CaptureStarted);
        h.deliver(1, EventType::ReceiveStarted);
        h.actor.onRequest({2, 1, Command::B, "a"});
        h.deliver(1, EventType::ImageArrived);
        check(h.responses.empty());
        h.deliver(1, EventType::ReceiveStopped);
        check(h.responses.at(0).second == "B image received");

        h.deliver(2, EventType::SystemReady);
        h.deliver(2, EventType::ControlDone);
        h.deliver(2, EventType::CaptureStarted);
        h.deliver(2, EventType::ReceiveStarted);
        h.deliver(2, EventType::ImageArrived);
        check(h.responses.size() == 1);
        h.deliver(2, EventType::ReceiveStopped);
        check(h.responses.at(1).first == 2);
    }
    {
        Harness h;
        h.actor.onRequest({1, 1, Command::B, "a"});
        h.deliver(1, EventType::SystemReady);
        h.deliver(1, EventType::ControlDone);
        h.deliver(1, EventType::CaptureStarted);
        h.actor.onRequest({2, 1, Command::A, "b"});
        h.actor.onRequest({3, 1, Command::C, "a"});
        check(h.responses.at(0).second == "cancelled");

        h.deliver(1, EventType::ReceiveStarted);
        h.deliver(1, EventType::ImageArrived);
        h.actor.onEvent({3, 2, EventType::ReceiveStopped});
        check(h.responses.size() == 1);
        h.deliver(3, EventType::ReceiveStopped);
        check(h.responses.at(1).first == 3);
        h.deliver(2, EventType::SystemReady);
        h.deliver(2, EventType::ControlDone);
        check(h.responses.at(2).first == 2);
    }
    {
        Harness h;
        h.actor.onRequest({1, 1, Command::B, "a"});
        // Hold the system acknowledgement until C has stopped the procedure.
        auto delayed = h.take(1, EventType::SystemReady);
        h.actor.onRequest({2, 1, Command::C, "a"});
        h.deliver(2, EventType::ReceiveStopped);
        h.actor.onEvent(delayed);
        check(h.responses.size() == 2);
        check(h.responses.at(0).second == "cancelled");
        check(h.responses.at(1).second == "C receive stopped");
        h.actor.onRequest({3, 1, Command::A, "a"});
        h.deliver(3, EventType::SystemReady);
        h.deliver(3, EventType::ControlDone);
        check(h.responses.at(2).second == "A completed");
    }
    {
        Harness h;
        h.actor.onRequest({2, 1, Command::D, "a", WriteOperation::Start, 42});
        h.deliver(2, EventType::WriteAccepted);
        check(h.responses.at(0).second == "D accepted (writing continues)");
        h.actor.onRequest({3, 1, Command::D, "b", WriteOperation::Query, 99});
        check(h.responses.back().second == "D unknown session");
        h.actor.onRequest({4, 1, Command::D, "a", WriteOperation::Start, 43});
        check(h.responses.back().second == "D session busy");
        h.actor.onRequest({5, 1, Command::D, "b", WriteOperation::Query, 42});
        h.deliver(5, EventType::WriteStatus);
        check(h.responses.back().second == "D complete");
    }
    {
        Harness h;
        h.actor.onRequest({1, 1, Command::A, "a"});
        h.actor.onEvent({1, 1, EventType::Failed});
        check(h.responses.back().second == "failed");
        h.deliver(1, EventType::SystemReady);
        check(h.responses.size() == 1);
    }
    {
        int count = 0;
        Coordinator coordinator([&](const Request&, const std::string&) { ++count; });
        coordinator.addCamera(1, std::make_unique<ModelAStrategy>());
        coordinator.addCamera(2, std::make_unique<ModelBStrategy>());
        coordinator.postRequest({1, 1, Command::B, "a"});
        coordinator.postRequest({2, 2, Command::D, "b", WriteOperation::Start, 7});
        coordinator.postRequest({3, 2, Command::D, "c", WriteOperation::Query, 7});
        coordinator.postRequest({4, 99, Command::A, "a"});
        while (count < 4) check(coordinator.runOnce());
        coordinator.stop();
        check(!coordinator.runOnce());
        // Destructor joins all five workers before the inbox is destroyed.
    }
    {
        Coordinator coordinator;
        coordinator.addCamera(1, std::make_unique<ModelAStrategy>());
        coordinator.postRequest({1, 1, Command::B, "a"});
        coordinator.runOnce(); // Destroy while worker activity is pending.
    }
}
