//===- StripTls.cpp - Remove the thread_local attribute from variables-----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// XXX
//
//===----------------------------------------------------------------------===//

// #include "llvm/IR/Function.h"
// #include "llvm/IR/Instructions.h"
// #include "llvm/IR/Intrinsics.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
// #include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/NVMPass.h"

using namespace llvm;

namespace {
// This is a ModulePass so that XXX...
class StripTls : public ModulePass {
public:
  static char ID; // Pass identification, replacement for typeid
  StripTls() : ModulePass(ID) {
    initializeStripTlsPass(*PassRegistry::getPassRegistry());
  }

  virtual bool runOnModule(Module &M);
};
} // namespace

char StripTls::ID = 0;
INITIALIZE_PASS(StripTls, "strip-tls",
                "Remove the thread_local attribute from variables", false,
                false)

bool StripTls::runOnModule(Module &M) {
  bool Changed = false;
  for (Module::global_iterator GV = M.global_begin(), E = M.global_end();
       GV != E; ++GV) {
    if (GV->isThreadLocal()) {
      GV->setThreadLocal(false);
      Changed = true;
    }
  }
  return Changed;
}

ModulePass *llvm::createStripTlsPass() { return new StripTls(); }
