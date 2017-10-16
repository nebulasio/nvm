//===- SandboxIndirectCalls.cpp - Add CFI to indirect function calls-------===//
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

#include "llvm/IR/Constants.h"
// #include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
// #include "llvm/IR/Intrinsics.h"
#include "llvm/IR/Module.h"
// #include "llvm/IR/Type.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/NVMPass.h"

using namespace llvm;

namespace {
// This is a ModulePass so that XXX...
class SandboxIndirectCalls : public ModulePass {
public:
  static char ID; // Pass identification, replacement for typeid
  SandboxIndirectCalls() : ModulePass(ID) {
    initializeSandboxIndirectCallsPass(*PassRegistry::getPassRegistry());
  }

  virtual bool runOnModule(Module &M);
};
} // namespace

char SandboxIndirectCalls::ID = 0;
INITIALIZE_PASS(SandboxIndirectCalls, "sandbox-indirect-calls",
                "Add CFI to indirect function calls", false, false)

bool SandboxIndirectCalls::runOnModule(Module &M) {
  Type *I32 = Type::getInt32Ty(M.getContext());
  Type *IntPtrType = I32; // XXX
  PointerType *PtrType = Type::getInt8Ty(M.getContext())->getPointerTo();

  SmallVector<Constant *, 20> FuncTable;
  // Reserve index 0.
  FuncTable.push_back(ConstantPointerNull::get(PtrType));

  // Build a function table out of address-taken functions.
  for (Module::iterator Func = M.begin(), E = M.end(); Func != E; ++Func) {
    // Look for address-taking references to the function.
    typedef SmallVector<Value::use_iterator, 10> Users_t;
    Users_t Users;
    for (Value::use_iterator U = Func->use_begin(), E = Func->use_end(); U != E;
         ++U) {
      if (CallInst *Call = dyn_cast<CallInst>(*U)) {
        // In PNaCl's normal form, a function referenced by a CallInst
        // can only appear as the callee, not an argument.
        if (U->getOperandNo() != Call->getNumArgOperands()) {
          errs() << "Value: " << **U << "\n";
          report_fatal_error("SandboxIndirectCalls: Bad function reference");
        }
      } else {
        // In PNaCl's normal form, all other references are PtrToInt
        // instructions or ConstantExprs.
        if (!(isa<PtrToIntInst>(*U) ||
              (isa<ConstantExpr>(*U) &&
               cast<ConstantExpr>(*U)->getOpcode() == Instruction::PtrToInt))) {
          errs() << "Value: " << **U << "\n";
          report_fatal_error("SandboxIndirectCalls: Bad function reference");
        }
        Users.push_back(U);
      }
    }

    // If the function is address-taken, allocate it an ID by adding
    // it to the function table.
    if (!Users.empty()) {
      Value *FuncIndex = ConstantInt::get(IntPtrType, FuncTable.size());
      // XXX: Remove bitcast when we use multiple tables.
      FuncTable.push_back(ConstantExpr::getBitCast(&(*Func), PtrType));

      for (Users_t::iterator U = Users.begin(), E = Users.end(); U != E; ++U) {
        Users_t::value_type VU = *U;
        (*VU)->replaceAllUsesWith(FuncIndex);
        // XXX: assumes cast is only used once.
        if (Instruction *Inst = dyn_cast<PtrToIntInst>(*VU))
          Inst->eraseFromParent();
      }
    }
  }

  Constant *TableArray =
      ConstantArray::get(ArrayType::get(PtrType, FuncTable.size()), FuncTable);
  Value *FuncTableGV = new GlobalVariable(
      M, TableArray->getType(), /*isConstant=*/true,
      GlobalVariable::InternalLinkage, TableArray, "__sfi_function_table");

  // Convert indirect function call instructions.
  for (Module::iterator Func = M.begin(), E = M.end(); Func != E; ++Func) {
    for (Function::iterator BB = Func->begin(), E = Func->end(); BB != E;
         ++BB) {
      for (BasicBlock::iterator Inst = BB->begin(), E = BB->end(); Inst != E;
           ++Inst) {
        assert(!isa<InvokeInst>(Inst));
        if (CallInst *Call = dyn_cast<CallInst>(Inst)) {
          Value *Callee = Call->getCalledValue();
          if (!isa<Function>(Callee)) {
            // assert...
            IntToPtrInst *Cast = cast<IntToPtrInst>(Callee);
            Value *FuncIndex = Cast->getOperand(0);

            Value *Indexes[] = {ConstantInt::get(I32, 0), FuncIndex};
            Value *Ptr = GetElementPtrInst::Create(
                Callee->getType(), FuncTableGV, Indexes, "func_gep", Call);
            Value *FuncPtr = new LoadInst(Ptr, "func", Call);
            // XXX: Remove bitcast when we use multiple tables.
            Value *Bitcast =
                new BitCastInst(FuncPtr, Cast->getType(), "func_bc", Call);
            Call->setCalledFunction(Bitcast);
            // XXX: assumes cast is only used once.
            Cast->eraseFromParent();
          }
        }
      }
    }
  }

  return true;
}

ModulePass *llvm::createSandboxIndirectCallsPass() {
  return new SandboxIndirectCalls();
}
