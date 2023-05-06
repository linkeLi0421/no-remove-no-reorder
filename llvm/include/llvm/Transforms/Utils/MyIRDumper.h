//===- llvm/include/llvm/Transforms/Utils/MyIRDumper.h ----------*- C++ -*-===//

#ifndef LLVM_TRANSFORMS_UTILS_MyIRDumper_H
#define LLVM_TRANSFORMS_UTILS_MyIRDUmper_H

#include "llvm/IR/PassManager.h"

namespace llvm {
class Module;
class MyIRDumperPass : public PassInfoMixin<MyIRDumperPass> {
public:
    MyIRDumperPass();
    PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM);
};
}

#endif