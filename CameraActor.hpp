#pragma once
#include "ProcedurePorts.hpp"
#include <deque>
#include <optional>
#include <utility>
struct WriteSession { uint64_t id; WriteState state; };
struct Procedure {
    enum class Stage { System, Config, Capture, Receive, Image, Stop, Write, Query } stage;
    Request request;
};
class CameraActor {
    int id_;
    ProcedurePorts ports_;
    std::deque<Request> pending_;
    std::optional<Procedure> procedure_;
    std::optional<WriteSession> session_;
    bool receiving_ = false;
    void startNext();
    void finish(const std::string& result);
public:
    CameraActor(int id, ProcedurePorts ports) : id_(id), ports_(std::move(ports)) {}
    void onRequest(Request request);
    void onEvent(const Event& event);
};
