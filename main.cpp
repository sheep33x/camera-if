#include "Coordinator.hpp"
#include <memory>
#include <thread>
#include <chrono>
int main() {
    Coordinator coordinator;
    coordinator.addCamera(1,std::make_unique<ModelAStrategy>());
    coordinator.addCamera(2,std::make_unique<ModelBStrategy>());
    // 通信スレッドA/Bの代わりに、異なるresponseRouteを持つ要求を投入。
    coordinator.postRequest({1,1,Command::B,"route-A"});
    coordinator.postRequest({3,2,Command::A,"route-B"});
    coordinator.postRequest({5,1,Command::C,"route-A"});
    coordinator.postRequest({7,2,Command::D,"route-B"}); // D開始 (奇数ID)
    coordinator.postRequest({8,2,Command::D,"route-B"}); // D照会 (偶数ID)
    // 受信完了イベントは実際には受信SDKのcallbackから送る。
    // coordinator.postImage(1);
    // このサンプルは構成説明用。runOnceはブロックするので、終了制御は省略。
    for (;;) coordinator.runOnce();
}
