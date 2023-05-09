#include <assert.h>
#include <stdio.h>

#include <iostream>
#include <map>
#include <vector>
#include <set>
#include <sys/stat.h>

#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/IRDumper.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/InitializePasses.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Transforms/Utils.h"


using namespace llvm;

namespace{
	class IRDumper : public ModulePass {

	public:
		static char ID;
		IRDumper() : ModulePass(ID) {
			initializeIRDumperPass(*PassRegistry::getPassRegistry());
		}
	};
}

static inline bool file_exist (const std::string& name) {
  struct stat buffer;   
  return (stat (name.c_str(), &buffer) == 0); 
}

static void saveModule(Module &M)
{
    // create SClog dir
    if (!file_exist("./IR")){
        int isCreate = mkdir("./IR", S_IRWXU);
        if(isCreate)
            outs() << "file create failed.\n";
    }
    std::string filename = M.getName().str();
    std::string file = filename.substr(filename.find('/') + 1);
    std::replace(file.begin(), file.end(), '/', '_');
	int bc_fd;
	sys::fs::openFileForWrite(Twine("./IR/" + file + ".bc"), bc_fd);
	raw_fd_ostream bc_file(bc_fd, true, true);
	WriteBitcodeToFile(M, bc_file);
}

char IRDumper::ID = 0;

PreservedAnalyses IRDumperPass::run(Module &M, ModuleAnalysisManager &AM) {
    saveModule(M);
    return PreservedAnalyses::all();
}
