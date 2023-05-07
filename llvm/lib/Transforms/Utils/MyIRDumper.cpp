//===- MyIRDumper.cpp - MyIRDumper -------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//
#include "llvm/Transforms/Utils/MyIRDumper.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/InitializePasses.h"
#include "llvm/Pass.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/IR/CFG.h"
#include "IRInfo.pb.h"
#include "llvm/IR/InstIndex.h"
#include "llvm/Transforms/Utils.h"
#include "llvm/IR/Instruction.h"
#include <string>
#include <fstream>
#include <ostream>
#include <string>
#include <sys/stat.h>
#include "llvm/IR/DebugInfoMetadata.h"

using namespace llvm;

typedef std::list<std::tuple<std::string,unsigned,unsigned>> DebugInfoList;

/// get the DebugInfoList for the instr and save it in the DIList
int getSrclineTree1(const Instruction *I, DebugInfoList &DIList) {
  if (!I)
    return -1;
  if (const DebugLoc &debugInfo = I->getDebugLoc()) {
    DebugLoc debugInfoTmp = debugInfo;
    do {
      auto *Scope = cast<DIScope>(debugInfoTmp.getScope());
      std::string filename = (std::string)(*Scope).getFilename();
      unsigned linenub = debugInfoTmp.getLine();
      unsigned colnub = debugInfoTmp.getCol();
      DIList.push_back(make_tuple(filename, linenub, colnub));
    } while ((debugInfoTmp = debugInfoTmp.getInlinedAt()));
    return 0;
  } else {
    return -1;
  }
}

namespace {
  class MyIRDumper : public FunctionPass {
  public:
    static char ID;
    MyIRDumper() : FunctionPass(ID) {
        initializeMyIRDumperPass(*PassRegistry::getPassRegistry());
        IR_func_book = new irpb::IRFunctionBook();
    }

    irpb::IRFunctionBook* IR_func_book;
    std::string arch;
    std::string filepath;
  };
}

char MyIRDumper::ID = 0;

std::string getBBLabel(const llvm::BasicBlock *Node) {
    if (!Node) 
        return "NULL_BB";

    if (!Node->getName().empty()) {
        return Node->getName().str();
    }

    std::string Str;
    llvm::raw_string_ostream OS(Str);
    Node->printAsOperand(OS, false);
    return OS.str();
}

inline bool file_exist (const std::string& name) {
  struct stat buffer;   
  return (stat (name.c_str(), &buffer) == 0); 
}

PreservedAnalyses MyIRDumperPass::run(Module &M, ModuleAnalysisManager &AM) {
    // initialization
    // test protobuf
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    // get arch from Targrt Triple
    std::string triple = M.getTargetTriple();
    char* triple_buffer = new char[triple.size() + 1];
    std::strcpy(triple_buffer, triple.c_str());
    std::string delim = "-";
    char *arch_p = strtok(triple_buffer, delim.c_str());
    std::string arch(arch_p);
    irpb::IRFunctionBook* IR_func_book = new irpb::IRFunctionBook();
    int tag_flag = 0;

    // set arch
    IR_func_book->set_arch(arch);

    // dump by functions
    for (auto &F : M) {
        const std::string &FName = F.getName().str();
        irpb::IRFunction *FMsg = IR_func_book->add_fs();
        FMsg->set_funcname(FName);

        // repeated IRBasicBlock MBBs
        for (auto &BB : F) {
        irpb::IRBasicBlock *BBMsg = FMsg->add_bbs();
        const std::string &BBID = getBBLabel(&BB);
        BBMsg->set_bblabel(BBID);

        // repeated IRInst MIs
        for (auto &I : BB) {
            if (!I.getMetadata("noreorder") && !I.getMetadata("noremove")) {
                // not tagged in source code
                continue;
            }
            tag_flag = 1;
            if(I.isDebugOrPseudoInst())
                continue;
            irpb::IRInst *IMsg = BBMsg->add_is();

            // tag noremove
            if (MDNode *N = I.getMetadata("noremove")) {
                IMsg->set_testmode("noremove");
                IMsg->set_removetagname(cast<MDString>(N->getOperand(0))->getString().str());
            }
            // tag noreorder
            if (MDNode *N = I.getMetadata("noreorder")) {
                IMsg->set_testmode("noreorder");
                for (unsigned int i = 0; i < N->getNumOperands(); i++) {
                    llvm::Metadata* Data = N->getOperand(i).get();
                    // Use the metadata as needed
                    if (auto s = dyn_cast<MDString>(Data)) {
                        IMsg->set_reordertagname(s->getString().str());
                    }
                    else if (auto index = dyn_cast<ConstantAsMetadata>(Data)) {
                        ConstantInt* const_index = cast<ConstantInt>(index->getValue());
                        unsigned tag_num = const_index->getZExtValue();
                        IMsg->set_reordertagnum(tag_num);
                    }
                }
            }

            std::string opcodeName = I.getOpcodeName();
            IMsg->set_opcode(opcodeName);
            signed operandCount = I.getNumOperands();
            if (operandCount) {
                for(int i=0; i<operandCount; i++) {
                    std::string operandName;
                    if (Instruction* Instr = dyn_cast<Instruction>(I.getOperand(i))) {
                        operandName = Instr->getOpcodeName();
                    }
                    else
                        operandName = I.getOperand(i)->getName().str();
                    IMsg->add_oprand(operandName);
                }
            }


            DebugLoc DL = I.getDebugLoc();
            InstIndex *II = I.getInstIndex();
            InstIndexSet IIS = I.getInstIndexSet();

            if (II) {
                irpb::InstIndex *IIMsg = new irpb::InstIndex();
                std::string funcName = II->getFuncName();
                std::string bbLabel = II->getBBLabel();
                unsigned instNo = II->getInstNum();
                IIMsg->set_funcname(funcName);
                IIMsg->set_bblabel(bbLabel);
                IIMsg->set_instno(instNo);
                IMsg->set_allocated_idx(IIMsg);
            }


            irpb::InstIndexList *IISMsg = new irpb::InstIndexList();
            InstIndexSet::iterator it = IIS.begin();
            // repeated InstIndex
            for (; it != IIS.end(); ++it) {
                if (*it == nullptr) continue;
                irpb::InstIndex *tmpII = IISMsg->add_idxs();
                std::string tmpfuncName = (*it)->getFuncName();
                std::string tmpbbLabel = (*it)->getBBLabel();
                unsigned tmpinstNo = (*it)->getInstNum();
                tmpII->set_funcname(tmpfuncName);
                tmpII->set_bblabel(tmpbbLabel);
                tmpII->set_instno(tmpinstNo);
            }
            IMsg->set_allocated_idxs(IISMsg);


            DebuginfoList DIL;
            // bool status = false;
            // I.getDebugInfoTree(DIL, status);
            int status = getSrclineTree1(&I, DIL);
            bool getfilepath = false;
            if (status==0) {
                for (auto &DebugInfo : DIL) {
                    irpb::InstLoc *LocMsg = IMsg->add_locs();
                    LocMsg->set_filename(std::get<0>(DebugInfo));
                    LocMsg->set_lineno(std::get<1>(DebugInfo));
                    LocMsg->set_colno(std::get<2>(DebugInfo));
                    if (!getfilepath){
                        getfilepath = true;
                    }
                }
            }
        }

        // repeated string SuccMBBLabel
        for (BasicBlock *S : successors(&BB)) {
            const std::string &SuccBB = S->getName().str();
            BBMsg->add_succbblabel(SuccBB); 
        }

            // repeated string PredMBBLabel
            for (BasicBlock *P : predecessors(&BB)) {
                const std::string &PredBB = P->getName().str();
                BBMsg->add_predbblabel(PredBB); 
            }

            }
    }

    if (tag_flag){
        // finalization
        // create IRlog dir
        if (!file_exist("./IRlog")){
            int isCreate = mkdir("./IRlog", S_IRWXU);
            if(isCreate)
                outs() << "file ./IRlog create failed.\n";
        }
        // create arch dir
        if (!file_exist("./IRlog/" + arch)){
            int isCreate = mkdir(("./IRlog/" + arch).c_str(), S_IRWXU);
            if(isCreate)
                outs() << "file "<< ("./IRlog/" + arch).c_str() <<" create failed.\n";
        }

        std::string filename = M.getName().str();
        std::string file = filename.substr(filename.find('/') + 1);
        std::replace(file.begin(), file.end(), '/', '_');
        std::fstream output("./IRlog/" + arch + "/" + file + ".IRInfo.log", std::ios::out | std::ios::trunc | std::ios::binary);
        if (!IR_func_book->SerializePartialToOstream(&output)) {
            outs() << "Failed to write IR msg. \n";
            output.close();
        }
        output.close();
    }

    return PreservedAnalyses::all();
}

MyIRDumperPass::MyIRDumperPass(){
}