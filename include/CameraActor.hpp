#pragma once
#include "Types.hpp"
#include <deque>
#include <optional>
#include <utility>
class SystemWorker;
class DeviceWorker;
class StreamWorker;
struct WriteSession { uint64_t id; WriteState state; };
struct Procedure {
    enum class Stage { System, Config, Capture, Receive, Image, Stop, Write, Query } stage;
    Request request;
};
class CameraActor {
    int id_;
    SystemWorker& system_;
    DeviceWorker& device_;
    StreamWorker& stream_;
    Respond respond_;
    std::deque<Request> pending_;
    std::optional<Procedure> procedure_;
    std::optional<WriteSession> session_;
    bool receiving_ = false;
    void startNext();
    void finish(const std::string& result);
public:
    // Workers must outlive the actor. These calls enqueue work on their owner threads.
    CameraActor(int id, SystemWorker& system, DeviceWorker& device,
                StreamWorker& stream, Respond respond)
        : id_(id), system_(system), device_(device), stream_(stream),
          respond_(std::move(respond)) {}
    void onRequest(Request request);
    void onEvent(const Event& event);
};
