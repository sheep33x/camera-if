#pragma once
#include "Types.hpp"
#include <functional>
#include <string>
using Respond = std::function<void(const Request&, const std::string&)>;
// Called only by the coordinator thread. No SDK handle crosses these ports.
struct ProcedurePorts {
    std::function<void(Action)> system;
    std::function<void(Action)> device;
    std::function<void(Action)> stream;
    Respond respond;
};
