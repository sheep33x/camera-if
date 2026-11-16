#pragma once
#include "Types.hpp"
#include <functional>
#include <memory>
#include <string>
using Dispatch = std::function<void(Action)>;
using Respond = std::function<void(const Request&, const std::string&)>;
class ICommandHandler {
protected:
    Request req_;
    Dispatch dispatch_;
    Respond respond_;
    bool finished_ = false;
    void send(const std::string& s) { if (!finished_) { finished_=true; respond_(req_,s); } }
public:
    ICommandHandler(Request r, Dispatch d, Respond out) : req_(std::move(r)),dispatch_(std::move(d)),respond_(std::move(out)) {}
    virtual ~ICommandHandler() = default;
    virtual void start()=0;
    virtual void onEvent(const Event&)=0;
    virtual void cancel() { send("cancelled"); }
    bool finished() const { return finished_; }
};
std::unique_ptr<ICommandHandler> makeHandler(Request r, Dispatch d, Respond out);
