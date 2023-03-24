//===-- InstIndex.cpp - Implement InstIndex class ---------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/IR/InstIndex.h"
#include "llvm/Config/llvm-config.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/Support/Debug.h"
using namespace llvm;

//===----------------------------------------------------------------------===//
// InstIndex Implementation
//===----------------------------------------------------------------------===//

#if !defined(NDEBUG) || defined(LLVM_ENABLE_DUMP)
LLVM_DUMP_METHOD void InstIndex::dump() const { print(dbgs()); }
#endif

void InstIndex::print(raw_ostream &OS) const {
  OS << FuncName << '@' << BBLabel << '@' << InstNum << ", label:" << MathFromSelect << ' ' << TailMerged << ' ' << TailDuplicated;
}