#pragma once
#include <cstdint>
#include <functional>
#include <string>
enum class Command { A, B, C, D };
enum class WriteOperation { Start, Query };
enum class WriteState { None, Writing, Complete, Error };
enum class EventType { SystemReady, ControlDone, CaptureStarted, ReceiveStarted, ImageArrived, ReceiveStopped, WriteAccepted, WriteStatus, Failed };
struct Request {
    uint64_t id; int cameraId; Command command; std::string responseRoute;
    WriteOperation writeOperation = WriteOperation::Start;
    uint64_t sessionId = 0;
};
using Respond = std::function<void(const Request&, const std::string&)>;
struct Event {
    uint64_t requestId; int cameraId; EventType type;
    WriteState writeState = WriteState::None;
};
struct Action {
    enum class Kind { PrepareSystem, ApplyConfig, StartCapture, StartReceive, StopReceive, StartWrite, QueryWrite } kind;
    uint64_t requestId; int cameraId;
};
