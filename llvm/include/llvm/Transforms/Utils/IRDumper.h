//===- llvm/include/llvm/Transforms/Utils/IRDumper.h ----------*- C++ -*-===//

#ifndef LLVM_TRANSFORMS_UTILS_IRDumper_H
#define LLVM_TRANSFORMS_UTILS_IRDUmper_H

#include "llvm/IR/PassManager.h"

namespace llvm {
class Function;
class IRDumperPass : public PassInfoMixin<IRDumperPass> {
public:
    PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM);

};
}

#endif