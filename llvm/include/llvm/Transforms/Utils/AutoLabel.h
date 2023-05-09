//===- llvm/include/llvm/Transforms/Utils/AutoLabel.h ----------*- C++ -*-===//

#ifndef LLVM_TRANSFORMS_UTILS_AutoLabel_H
#define LLVM_TRANSFORMS_UTILS_AutoLabel_H

#include "llvm/IR/PassManager.h"

namespace llvm {
class Module;
class AutoLabelPass : public PassInfoMixin<AutoLabelPass> {
public:
    AutoLabelPass();
    PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM);
};
}

#endif