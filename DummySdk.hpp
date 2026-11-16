#pragma once
#include <cassert>
#include <string>
#include <thread>
// Dummy handles enforce the same ownership rule as the future SDK adapters.
class OwnedSdkHandle {
    std::thread::id owner_ = std::this_thread::get_id();
public:
    void perform(const std::string&) { assert(owner_ == std::this_thread::get_id()); }
    ~OwnedSdkHandle() { assert(owner_ == std::this_thread::get_id()); }
};
using SystemSdkHandle = OwnedSdkHandle;
class ControlSdkHandle : public OwnedSdkHandle { public: explicit ControlSdkHandle(int) {} };
class ReceiveSdkHandle : public OwnedSdkHandle { public: explicit ReceiveSdkHandle(int) {} };
