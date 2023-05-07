//===- InstIndex.h - Instruction Index Information ------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file defines a number of light weight data structures used
// to describe the unique identification of an instruction.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_InstIndex
#define LLVM_InstIndex

#include <set>
#include <string>
#include <tuple>

namespace llvm {

  class raw_ostream;

  class InstIndex {
    // format: funcname, BBlabel, Inst num in bb
    std::string FuncName;
    std::string BBLabel;
    uint32_t InstNum;

  public:
    /// fields: used to represent what transformation this idx has gone through

    // To help judge if an inst is a cond-solver
    // FromSelect == 1: this inst trans from select
    bool MathFromSelect;
    // This 2 bool var can help mapping tailmerged insts(I1,I2,I3->I4)
    // if an mir inst Merged == 1 && Duplicated == 0, 
    // we can use InstIndexSet to map
    bool TailMerged;
    // duplicated by TailDuplicate
    bool TailDuplicated;

    InstIndex() { MathFromSelect = 0; TailMerged = 0; TailDuplicated = 0; };

    InstIndex(const InstIndex& SrcIndex) = default;

    InstIndex& operator = (const InstIndex& SrcIndex) = default;

    void setFuncName(std::string SrcFuncName) {
      FuncName = SrcFuncName;
    }

    void setBBLabel(std::string SrcBBLabel) {
      BBLabel = SrcBBLabel;
    }

    void setInstNum(uint32_t SrcInstNum) {
      InstNum = SrcInstNum;
    }

    std::string getFuncName() const {
      return FuncName;
    }

    std::string getBBLabel() const {
      return BBLabel;
    }

    int getInstNum() const {
      return InstNum;
    }

    bool operator==(const InstIndex &II) const { return FuncName == II.FuncName && BBLabel == II.BBLabel && InstNum == II.InstNum; }
    bool operator==(std::string s) const { return FuncName + '@' + BBLabel + '@' + std::to_string(InstNum) == s; }
    bool operator!=(const InstIndex &II) const { return !(FuncName == II.FuncName && BBLabel == II.BBLabel && InstNum == II.InstNum); }

    explicit operator bool() const { return FuncName != ""; }

    bool operator<(const InstIndex &II) const {
      if (FuncName != II.FuncName) {
        return FuncName < II.FuncName;
      }
      else if (BBLabel != II.BBLabel){
        return BBLabel < II.BBLabel;
      }
      else {
        return InstNum < II.InstNum;
      }
    }

    void dump() const;

    void print(raw_ostream &OS) const;

  };

  struct InstIndexHash {
    std::size_t operator()(const InstIndex& index) const noexcept {
      std::size_t h1 = std::hash<std::string>{}(index.getFuncName());
      std::size_t h2 = std::hash<std::string>{}(index.getBBLabel());
      std::size_t h3 = std::hash<uint32_t>{}(index.getInstNum());
      return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
  };

typedef std::set<InstIndex*> InstIndexSet;


} // end namespace llvm

#endif   