#pragma once
#include <string>
#include <iostream>
// 実際のSDKクラスではない。スレッド専有ハンドルのイメージを示す。
class ControlSdkHandle {
    int cameraId_;
public:
    explicit ControlSdkHandle(int cameraId) : cameraId_(cameraId) {}
    void perform(const std::string& description) { std::cout << "[ControlSDK camera=" << cameraId_ << "] " << description << "\n"; }
};
class ReceiveSdkHandle {
    int cameraId_;
public:
    explicit ReceiveSdkHandle(int cameraId) : cameraId_(cameraId) {}
    void perform(const std::string& description) { std::cout << "[ReceiveSDK camera=" << cameraId_ << "] " << description << "\n"; }
};
