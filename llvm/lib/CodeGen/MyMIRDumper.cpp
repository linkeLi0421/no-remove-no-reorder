//===- MyMIRDumper.cpp - MyMIRDumper -------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/InitializePasses.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/IR/CFG.h"
#include "MIRInfo.pb.h"
#include <string>
#include <fstream>
#include <ostream>
#include "llvm/Transforms/Utils.h"
#include "llvm/IR/InstIndex.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineFunction.h"
#include <sys/stat.h>


using namespace llvm;

namespace {
  class MyMIRDumper : public MachineFunctionPass {
  public:
    static char ID;

    MyMIRDumper() : MachineFunctionPass(ID) {
        initializeMyMIRDumperPass(*PassRegistry::getPassRegistry());
        MIR_func_book = new mirpb::MIRFunctionBook();
    }

    MyMIRDumper(std::string prePassName) : MachineFunctionPass(ID) {
        initializeMyMIRDumperPass(*PassRegistry::getPassRegistry());
        MIR_func_book = new mirpb::MIRFunctionBook();
        PrePassName = prePassName;
        std::replace(PrePassName.begin(), PrePassName.end(), ' ', '_');
    }

    bool runOnMachineFunction(MachineFunction &F) override;
    bool doFinalization(Module &M) override;
    bool doInitialization(Module &M) override;

    mirpb::MIRFunctionBook* MIR_func_book;
    std::string PrePassName;
    std::string arch;
    std::string filepath;
  };
}

char MyMIRDumper::ID = 0;

inline bool file_exist (const std::string& name) {
  struct stat buffer;   
  return (stat (name.c_str(), &buffer) == 0); 
}

// extern std::string getBBLabel(const BasicBlock *Node);
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


bool MyMIRDumper::runOnMachineFunction(MachineFunction &MF) {
    unsigned MFID = MF.getFunctionNumber();
    const std::string &MFName = MF.getName().str();
    mirpb::MIRFunction *MFMsg = MIR_func_book->add_mfs();
    MFMsg->set_funcname(MFName);
    MFMsg->set_funcid(MFID);

    // repeated MIRBasicBlock MBBs
    for (auto &MBB : MF) {
        mirpb::MIRBasicBlock *MBBMsg = MFMsg->add_mbbs();
        std::string MBBID = std::to_string(MBB.getNumber());
        MBBMsg->set_mbblabel(MBBID);
        const BasicBlock *BB = MBB.getBasicBlock();
        std::string BBID;
        if (!BB) {
            BBID = "NULL_BB";
        } else {
            BBID = getBBLabel(BB);
        }
        MBBMsg->set_bblabel(BBID);

        // repeated MIRInst MIs
        for (MachineBasicBlock::instr_iterator
         MI = MBB.instr_begin(), E = MBB.instr_end(); MI != E; ++MI) {
            // if(MI->isDebugInstr()) 
            //     continue;
            mirpb::MIRInst *MIMsg = MBBMsg->add_mis();

            std::string opcodeName = MI->getOpcodeName();
            MIMsg->set_opcode(opcodeName);
            std::string opType = MI->getOpType();
            MIMsg->set_optype(opType);
            // repeated string Oprand
            signed operandCount = MI->getNumOperands();
            if (operandCount){
                for(int i=0;i<operandCount;i++){
                    std::string operandName = MI->getOperand(i).getOperandName();
                    MIMsg->add_oprand(operandName);
                }
            }

            DebugLoc DL = MI->getDebugLoc();
            InstIndex *II = MI->getInstIndex();
            InstIndexSet IIS =  MI->getInstIndexSet();


            if (II != nullptr) {
                mirpb::InstIndex *IIMsg = new mirpb::InstIndex();
                std::string funcName = II->getFuncName();
                std::string bbLabel = II->getBBLabel();
                unsigned instNo = II->getInstNum();
                bool FromSelect = II->MathFromSelect;
                bool Merged = II->TailMerged;
                bool Duplicated = II->TailDuplicated;
                bool FromCopy = II->FromCopy;
                IIMsg->set_funcname(funcName);
                IIMsg->set_bblabel(bbLabel);
                IIMsg->set_instno(instNo);
                IIMsg->set_mathfromselect(FromSelect);
                IIMsg->set_tailmerged(Merged);
                IIMsg->set_tailduplicated(Duplicated);
                IIMsg->set_fromcopy(FromCopy);
                MIMsg->set_allocated_idx(IIMsg);
            }

            mirpb::InstIndexList *IISMsg = new mirpb::InstIndexList();
            InstIndexSet::iterator it = IIS.begin();
            // repeated InstIndex
            for (; it != IIS.end(); ++it) {
                if (*it == nullptr) continue; 
                mirpb::InstIndex *tmpII = IISMsg->add_idxs();
                std::string tmpfuncName = (*it)->getFuncName();
                std::string tmpbbLabel = (*it)->getBBLabel();
                unsigned tmpinstNo = (*it)->getInstNum();
                tmpII->set_funcname(tmpfuncName);
                tmpII->set_bblabel(tmpbbLabel);
                tmpII->set_instno(tmpinstNo);
            }
            MIMsg->set_allocated_idxs(IISMsg);

            DebuginfoList DIL;
            bool status = false;
            MI->getDebugInfoTree(DIL, status);
            bool getfilepath = false;
            if (status) {
                for (auto &DebugInfo : DIL) {
                    mirpb::InstLoc *LocMsg = MIMsg->add_locs();
                    LocMsg->set_filename(std::get<0>(DebugInfo));
                    LocMsg->set_lineno(std::get<1>(DebugInfo));
                    LocMsg->set_colno(std::get<2>(DebugInfo));
                    if (!getfilepath){
                        MyMIRDumper::filepath = std::get<0>(DebugInfo);
                        getfilepath = true;
                    }
                }
            }
        }

        // repeated string SuccMBBLabel
        for (MachineBasicBlock *S : MBB.successors()) {
            std::string SuccMBB = std::to_string(S->getNumber());
            MBBMsg->add_succmbblabel(SuccMBB); 
        }

        // repeated string PredMBBLabel
        for (MachineBasicBlock *P : MBB.predecessors()) {
            std::string PredMBB = std::to_string(P->getNumber());
            MBBMsg->add_predmbblabel(PredMBB); 
        }
    }
    return false;
}

bool MyMIRDumper::doInitialization(Module &M) {
    // test protobuf
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    // get arch from Targrt Triple
    std::string triple = M.getTargetTriple();
    char* triple_buffer = new char[triple.size() + 1];
    std::strcpy(triple_buffer, triple.c_str());
    std::string delim = "-";
    char *arch_p = strtok(triple_buffer, delim.c_str());
    std::string arch(arch_p);
    MyMIRDumper::arch = arch;

    // set arch
    MIR_func_book->set_arch(arch);
    return false;
}

bool MyMIRDumper::doFinalization(Module &M) {
    // create MIRlog dir
    if (!file_exist("./MIRlog")){
        int isCreate = mkdir("./MIRlog", S_IRWXU);
        if(isCreate)
            outs() << "file ./MIRlog create failed.\n";
    }
    // create arch dir
    if (!file_exist("./MIRlog/" + MyMIRDumper::arch)){
        int isCreate = mkdir(("./MIRlog/" + MyMIRDumper::arch).c_str(), S_IRWXU);
        if(isCreate)
            outs() << "file "<< ("./MIRlog/" + MyMIRDumper::arch).c_str() <<" create failed.\n";
    }
    // create pass dir
    std::replace(MyMIRDumper::PrePassName.begin(), MyMIRDumper::PrePassName.end(), '/', '_');
    std::replace(MyMIRDumper::PrePassName.begin(), MyMIRDumper::PrePassName.end(), '&', '_');
    std::replace(MyMIRDumper::PrePassName.begin(), MyMIRDumper::PrePassName.end(), '(', '_');
    std::replace(MyMIRDumper::PrePassName.begin(), MyMIRDumper::PrePassName.end(), ')', '_');
    std::replace(MyMIRDumper::PrePassName.begin(), MyMIRDumper::PrePassName.end(), '\'', '_');
    std::replace(MyMIRDumper::PrePassName.begin(), MyMIRDumper::PrePassName.end(), '>', '_');
    if (!file_exist("./MIRlog/" + MyMIRDumper::arch + "/" + MyMIRDumper::PrePassName)){
        int isCreate = mkdir(("./MIRlog/" + MyMIRDumper::arch + "/" + MyMIRDumper::PrePassName).c_str(), S_IRWXU);
        if(isCreate)
            outs() << "file "<< ("./MIRlog/" + MyMIRDumper::arch + "/" + MyMIRDumper::PrePassName).c_str() <<" create failed.\n";
    }
    std::string filename = M.getName().str();
    std::string file = filename.substr(filename.find('/') + 1);
    std::replace(file.begin(), file.end(), '/', '_');
    std::fstream output("./MIRlog/" + MyMIRDumper::arch + "/" + MyMIRDumper::PrePassName 
        + "/" + file + ".MIRInfo.log", std::ios::out | std::ios::trunc | std::ios::binary);
    if (!MIR_func_book->SerializePartialToOstream(&output)) {
        outs() << "Failed to write msg. \n";
        output.close();
        return true;
    }
    output.close();
    return false;
} 

INITIALIZE_PASS(MyMIRDumper, "MyMIRDumper",
                "Dump MIR Info before CodeGen", false, false)

MachineFunctionPass *llvm::createMyMIRDumperPass(std::string prePassName) {
    return new MyMIRDumper(prePassName);
}  