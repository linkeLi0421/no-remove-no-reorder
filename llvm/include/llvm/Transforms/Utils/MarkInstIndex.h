//===- llvm/include/llvm/Transforms/Utils/MarkInstIndex.h ----------*- C++ -*-===//

#ifndef LLVM_TRANSFORMS_UTILS_MarkInstIndex_H
#define LLVM_TRANSFORMS_UTILS_MarkInstIndex_H

#include "llvm/IR/PassManager.h"

namespace llvm {
class Function;
class MarkInstIndexPass : public PassInfoMixin<MarkInstIndexPass> {
private:
    std::string PassName;
public:
    PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
};
}

#endif