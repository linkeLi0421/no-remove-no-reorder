//===- AutoLabel.cpp - AutoLabel -------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//
#include "llvm/Transforms/Utils/AutoLabel.h"
#include "llvm/InitializePasses.h"
#include "llvm/Pass.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/InstIndex.h"
#include "llvm/IR/Constants.h"
#include "SecurityCheck.pb.h"
#include <fstream>
#include <cstdlib>
#include <string>
#include <map>
#include <set>

using namespace llvm;

namespace {
  class AutoLabel : public FunctionPass {
  public:
    static char ID;
    AutoLabel() : FunctionPass(ID) {
        initializeAutoLabelPass(*PassRegistry::getPassRegistry());
    }
  };
}

char AutoLabel::ID = 0;

std::string getArch(Module& M) {
    // get arch from Targrt Triple
    std::string triple = M.getTargetTriple();
    char* triple_buffer = new char[triple.size() + 1];
    std::strcpy(triple_buffer, triple.c_str());
    std::string delim = "-";
    char *arch_p = strtok(triple_buffer, delim.c_str());
    std::string arch(arch_p);
    return arch;
}

std::string getFilename(Module& M) {
    std::string filename = M.getName().str();
    std::string file = filename.substr(filename.find('/') + 1);
    std::replace(file.begin(), file.end(), '/', '_');
    return file;
}

PreservedAnalyses AutoLabelPass::run(Module &M, ModuleAnalysisManager &AM) {
    // read security checks from file
    pb::SecurityCheckBook* SC_book= new pb::SecurityCheckBook();
    std::string sc_pblog_dir = std::getenv("SC_PBLOG_DIR");
    std::string filename = getFilename(M);
    std::string arch = getArch(M);
    std::map<std::string, InstIndex> noremove_map;
    std::map<std::string, std::vector<InstIndex>> noreorder_map;
    std::unordered_set<InstIndex, InstIndexHash> indexset;
    std::fstream in(sc_pblog_dir + "/" + arch + "/" + filename + ".SCInfo.log", std::ios::in|std::ios::binary);
    if (!SC_book->ParseFromIstream(&in)) {
      errs()<<"error in read scpb in "<<sc_pblog_dir + "/" + arch + "/"  + filename + ".SCInfo.log"<<"\n";
    }

    for (int i = 0; i < SC_book->securitychecks_size(); i++) {
      // use securtiry check's cbr instindex as tag name (key in map)
      std::string tag_name = SC_book->securitychecks(i).funcname() + 
                             SC_book->securitychecks(i).brbblabel() +
                             std::to_string(SC_book->securitychecks(i).brindex());
      InstIndex instindex;
      instindex.setFuncName(SC_book->securitychecks(i).funcname());
      instindex.setBBLabel(SC_book->securitychecks(i).brbblabel());
      instindex.setInstNum(SC_book->securitychecks(i).brindex());
      // securitry check is tagged no remove and no reordered
      noremove_map.insert(std::make_pair(tag_name, instindex));
      noreorder_map[tag_name] = {instindex};
      indexset.insert(instindex);

      for (int j = 0; j < SC_book->securitychecks(i).firstreads_size(); j++) {
        pb::SecurityCheck_FirstRead FirstRead = SC_book->securitychecks(i).firstreads(j);
        // reads to checked varible is tagged to no reordered
        instindex.setFuncName(FirstRead.funcname());
        instindex.setBBLabel(FirstRead.bblabel());
        instindex.setInstNum(FirstRead.instno());
        noreorder_map[tag_name].push_back(instindex);
        indexset.insert(instindex);
      }
      
      // tag instructions using two maps
      for (auto &F : M) {
        for (auto &B : F) {
          for (auto &I : B) {
            InstIndex II = *I.getInstIndex();
            if (indexset.count(II)) {
              // mark the instruction
              for(auto i : noremove_map) {
                if (i.second == II) {
                  // find tag in noremove
                  llvm::StringRef tag_name = llvm::StringRef(i.first);
                  llvm::MDString* MD_tag_name = llvm::MDString::get(F.getContext(), tag_name);
                  I.setMetadata("noremove", llvm::MDNode::get(F.getContext(), MD_tag_name));
                }
              }
              for(auto i : noreorder_map) {
                if (std::find(i.second.begin(), i.second.end(), II) != i.second.end()) {
                  // find tag in noreorder
                  llvm::StringRef tag_name = llvm::StringRef(i.first);
                  llvm::MDString* MD_tag_name = llvm::MDString::get(F.getContext(), tag_name);
                  int index = std::distance(i.second.begin(), std::find(i.second.begin(), i.second.end(), II));
                  ConstantInt* const_index = ConstantInt::get(llvm::Type::getInt32Ty(F.getContext()), index);
                  std::vector<Metadata*> MetadataValues;
                  MetadataValues.push_back(ConstantAsMetadata::get(const_index));
                  MetadataValues.push_back(MD_tag_name);
                  I.setMetadata("noreorder", llvm::MDNode::get(F.getContext(), MetadataValues));
                }
              }
            }
          }
        }
      }

    }

    return PreservedAnalyses::all();
}

AutoLabelPass::AutoLabelPass(){
}