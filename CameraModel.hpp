#pragma once
#include <cstdint>
#include <string>
struct RegisterMap { uint32_t exposure; uint32_t gain; uint32_t trigger; };
// Strategyは機種ごとの「同じSDKのAPI選択・レジスタ・呼び出し順序」を表す。
// ここではSDKを実際には呼ばず、操作内容を文字列で説明する。
class CameraModelStrategy {
public:
    virtual ~CameraModelStrategy() = default;
    virtual std::string applyConfigRecipe() const = 0;
    virtual std::string captureRecipe() const = 0;
};
class ModelAStrategy final : public CameraModelStrategy {
public:
    std::string applyConfigRecipe() const override;
    std::string captureRecipe() const override;
};
class ModelBStrategy final : public CameraModelStrategy {
public:
    std::string applyConfigRecipe() const override;
    std::string captureRecipe() const override;
};
