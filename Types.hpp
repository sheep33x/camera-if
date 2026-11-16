#pragma once
#include <cstdint>
#include <string>

enum class Command { A, B, C, D };
enum class EventType { ControlDone, CaptureStarted, ReceiveStarted, ImageArrived, ReceiveStopped, WriteAccepted, WriteStatus, Failed };
enum class WriteState { None, Writing, Complete, Error };
struct Request { uint64_t id; int cameraId; Command command; std::string responseRoute; };
struct Event { uint64_t requestId; int cameraId; EventType type; WriteState writeState = WriteState::None; };
struct Action { enum class Kind { ApplyConfig, StartCapture, StartReceive, StopReceive, StartWrite, QueryWrite } kind; uint64_t requestId; int cameraId; };
