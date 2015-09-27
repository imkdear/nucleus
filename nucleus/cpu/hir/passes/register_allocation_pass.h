/**
 * (c) 2015 Alexandro Sanchez Bach. All rights reserved.
 * Released under GPL v2 license. Read LICENSE for more details.
 */

#pragma once

#include "nucleus/common.h"
#include "nucleus/cpu/backend/target.h"
#include "nucleus/cpu/hir/pass.h"

namespace cpu {
namespace hir {
namespace passes {

class RegisterAllocationPass : public Pass {
private:
    // Target information
    const backend::TargetInfo& targetInfo;

    // Handle call arguments
    void allocateArgument(int index, Value* value);

public:
    // Constructor
    RegisterAllocationPass(const backend::TargetInfo& targetInfo);

    // Get the name of this pass
    const char* name() override {
        return "Register Allocation";
    }

    // Apply this pass on a function
    bool run(Function* function) override;
};

}  // namespace passes
}  // namespace hir
}  // namespace cpu
