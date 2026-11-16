#include "CameraModel.hpp"
#include <sstream>
std::string ModelAStrategy::applyConfigRecipe() const {
    RegisterMap r{0x100, 0x104, 0x108};
    std::ostringstream s; s << "WriteReg(exposure=" << r.exposure << ") -> WriteReg(gain=" << r.gain << ")"; return s.str();
}
std::string ModelAStrategy::captureRecipe() const { return "WriteReg(trigger) -> SDK.StartCapture()"; }
std::string ModelBStrategy::applyConfigRecipe() const {
    RegisterMap r{0x200, 0x210, 0x220};
    std::ostringstream s; s << "SDK.SetMode() -> WriteReg(gain=" << r.gain << ") -> WriteReg(exposure=" << r.exposure << ")"; return s.str();
}
std::string ModelBStrategy::captureRecipe() const { return "SDK.PrepareCapture() -> SDK.StartCapture()"; }
