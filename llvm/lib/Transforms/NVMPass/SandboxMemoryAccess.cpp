//===- SandboxMemoryAccesses.cpp - Add SFI to memory accesses--------------===//
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

#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
// #include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/NVMPass.h"

using namespace llvm;

namespace {
// This is a ModulePass so that XXX...
class SandboxMemoryAccesses : public ModulePass {
  Value *MemBaseVar;
  Value *MemBase;

  Value *sandboxPtr(Value *Ptr, Instruction *InsertPt);
  void sandboxOperand(Instruction *Inst, unsigned OpNum);
  void convertFunc(Function *Func);

public:
  static char ID; // Pass identification, replacement for typeid
  SandboxMemoryAccesses() : ModulePass(ID) {
    initializeSandboxMemoryAccessesPass(*PassRegistry::getPassRegistry());
  }

  virtual bool runOnModule(Module &M);
};
} // namespace

char SandboxMemoryAccesses::ID = 0;
INITIALIZE_PASS(SandboxMemoryAccesses, "sandbox-memory-accesses",
                "Add SFI sandboxing to memory accesses", false, false)

Value *SandboxMemoryAccesses::sandboxPtr(Value *Ptr, Instruction *InsertPt) {
  if (!MemBase) {
    Function *Func = InsertPt->getParent()->getParent();
    Instruction *MemBaseInst = new LoadInst(MemBaseVar, "mem_base");
    Func->getEntryBlock().getInstList().push_front(MemBaseInst);
    MemBase = MemBaseInst;
  }

  Type *I32 = Type::getInt32Ty(InsertPt->getContext());
  Type *I64 = Type::getInt64Ty(InsertPt->getContext());

  // Look for the pattern produced by ExpandGetElementPtr.
  // TODO: ExpandGetElementPtr should really put a "nuw" attr on the
  // add, and we should check for this here.
  if (IntToPtrInst *Cast = dyn_cast<IntToPtrInst>(Ptr)) {
    if (BinaryOperator *Op = dyn_cast<BinaryOperator>(Cast->getOperand(0))) {
      if (Op->getOpcode() == Instruction::Add) {
        if (ConstantInt *CI = dyn_cast<ConstantInt>(Op->getOperand(1))) {
          uint64_t Addend = CI->getZExtValue();
          if (Addend < 0x10000) {
            Value *ZExt = new ZExtInst(Op->getOperand(0), I64, "", InsertPt);
            Value *Add1 = BinaryOperator::Create(BinaryOperator::Add, MemBase,
                                                 ZExt, "", InsertPt);
            Value *Add2 = BinaryOperator::Create(BinaryOperator::Add, Add1,
                                                 ConstantInt::get(I64, Addend),
                                                 "", InsertPt);
            return new IntToPtrInst(Add2, Ptr->getType(), "", InsertPt);
          }
        }
      }
    }
  }

  Value *Truncated = new PtrToIntInst(Ptr, I32, "", InsertPt);
  Value *ZExt = new ZExtInst(Truncated, I64, "", InsertPt);
  Value *Added =
      BinaryOperator::Create(BinaryOperator::Add, MemBase, ZExt, "", InsertPt);
  return new IntToPtrInst(Added, Ptr->getType(), "", InsertPt);
}

void SandboxMemoryAccesses::sandboxOperand(Instruction *Inst, unsigned OpNum) {
  Inst->setOperand(OpNum, sandboxPtr(Inst->getOperand(OpNum), Inst));
}

void SandboxMemoryAccesses::convertFunc(Function *Func) {
  MemBase = NULL;
  for (Function::iterator BB = Func->begin(), E = Func->end(); BB != E; ++BB) {
    for (BasicBlock::iterator Inst = BB->begin(), E = BB->end(); Inst != E;
         ++Inst) {
      if (isa<LoadInst>(Inst)) {
        sandboxOperand(&(*Inst), 0);
      } else if (isa<StoreInst>(Inst)) {
        sandboxOperand(&(*Inst), 1);
      } else if (isa<MemCpyInst>(Inst) || isa<MemMoveInst>(Inst)) {
        sandboxOperand(&(*Inst), 0);
        sandboxOperand(&(*Inst), 1);
      } else if (isa<MemSetInst>(Inst)) {
        sandboxOperand(&(*Inst), 0);
      }
    }
  }
}

bool SandboxMemoryAccesses::runOnModule(Module &M) {
  Type *I64 = Type::getInt64Ty(M.getContext());
  MemBaseVar = M.getOrInsertGlobal("__sfi_memory_base", I64);
  for (Module::iterator Func = M.begin(), E = M.end(); Func != E; ++Func) {
    convertFunc(&(*Func));
  }
  return true;
}

ModulePass *llvm::createSandboxMemoryAccessesPass() {
  return new SandboxMemoryAccesses();
}
