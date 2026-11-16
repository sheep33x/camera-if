#include "Coordinator.hpp"
#include <iostream>
int main() {
    int responses = 0;
    Coordinator coordinator([&](const Request& r, const std::string& result) {
        std::cout << r.id << ": " << result << '\n'; ++responses;
    });
    coordinator.addCamera(1,std::make_unique<ModelAStrategy>());
    coordinator.addCamera(2,std::make_unique<ModelBStrategy>());
    coordinator.postRequest({1,1,Command::B,"route-A"});
    coordinator.postRequest({2,2,Command::A,"route-B"});
    coordinator.postRequest({3,1,Command::C,"route-A"});
    coordinator.postRequest({4,2,Command::D,"route-B",WriteOperation::Start,100});
    coordinator.postRequest({5,2,Command::D,"route-B",WriteOperation::Query,100});
    while (responses < 5 && coordinator.runOnce()) {}
}
