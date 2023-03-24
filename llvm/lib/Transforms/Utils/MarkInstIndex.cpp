//===--- MarkInstIndex.cpp - Mark Inst Index --------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//
#include "llvm/IR/Module.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/InitializePasses.h"
#include "llvm/Pass.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/ModuleSlotTracker.h"
#include "llvm/Transforms/Utils.h"
#include "llvm/Transforms/Utils/MarkInstIndex.h"
#include <map>


using namespace llvm;

namespace {
    class MarkInstIndex : public FunctionPass {
    public:
        static char ID; // Pass identification, replacement for typeid
        MarkInstIndex() : FunctionPass(ID) {
        initializeMarkInstIndexPass(*PassRegistry::getPassRegistry());
        }

        bool runOnFunction(Function &F) override;
    };

}

static void MarkEveryInst(Function &F) {
    const std::string &funcname = F.getName().str();
    std::map<BasicBlock*, std::string> mp;
    for (auto &BB : F) {
        std::string bblabel;
        if (BB.hasName()) {
            bblabel = BB.getName().str();
        }
        else if (&BB != &F.getEntryBlock()){
            // to keep consistence with IR dumper
            const Module *M = F.getParent();
            ModuleSlotTracker MST(M);
            MST.incorporateFunction(F);
            int Slot = MST.getLocalSlot(&BB);
            bblabel = '%' + Twine(Slot).str();
            mp[&BB] = bblabel;

        }

        int index = 0;
        for (auto &Inst : BB) {
            ++index;
            InstIndex *tmpIndex = new InstIndex();
            DebugLoc tmp;
            tmpIndex->setFuncName(funcname);
            tmpIndex->setBBLabel(bblabel);
            tmpIndex->setInstNum(index);

            Inst.setInstIndex(tmpIndex);
        }
    }

    // mark index for those no-name BBs
    // cant do it in the former for loop as setName() will affect other no-name BB's label
    std::map<BasicBlock*, std::string>::iterator it = mp.begin();
    for (; it != mp.end(); ++it) {
        BasicBlock* thisbb = it->first;
        thisbb->setName(it->second);
    }

}

bool MarkInstIndex::runOnFunction(Function &F) {
        MarkEveryInst(F);
        return false;
}

char MarkInstIndex::ID = 0;

INITIALIZE_PASS(MarkInstIndex, "markinstindex",
                "Mark Inst Index before backend", false, false)

FunctionPass *llvm::createMarkInstIndexPass() {
  return new MarkInstIndex();
}  

PreservedAnalyses MarkInstIndexPass::run(Function &F, FunctionAnalysisManager &AM) {
    MarkEveryInst(F);
    return PreservedAnalyses::all();
}
