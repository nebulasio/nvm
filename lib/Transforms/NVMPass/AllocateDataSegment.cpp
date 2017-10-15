//===- AllocateDataSegment.cpp - Create template for data segment----------===//
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
#include "llvm/IR/DataLayout.h"
// #include "llvm/IR/Function.h"
// #include "llvm/IR/Instructions.h"
// #include "llvm/IR/Intrinsics.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
// #include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/NaCl.h"

using namespace llvm;

namespace {
// This is a ModulePass because it modifies global variables.
class AllocateDataSegment : public ModulePass {
public:
  static char ID; // Pass identification, replacement for typeid
  AllocateDataSegment() : ModulePass(ID) {
    initializeAllocateDataSegmentPass(*PassRegistry::getPassRegistry());
  }

  virtual bool runOnModule(Module &M);
};
} // namespace

char AllocateDataSegment::ID = 0;
INITIALIZE_PASS(AllocateDataSegment, "allocate-data-segment",
                "Create template for data segment", false, false)

bool AllocateDataSegment::runOnModule(Module &M) {
  Type *I32 = Type::getInt32Ty(M.getContext());
  Type *IntPtrType = I32; // XXX
  unsigned BaseAddr = 0x10000;

  SmallVector<Type *, 20> Types;
  for (Module::global_iterator GV = M.global_begin(), E = M.global_end();
       GV != E; ++GV) {
    if (!GV->hasInitializer()) {
      report_fatal_error(std::string("Variable ") + GV->getName() +
                         " has no initializer");
    }
    Types.push_back(GV->getInitializer()->getType());
  }
  StructType *TemplateType =
      StructType::create(M.getContext(), "data_template");
  // TODO: add padding manually and set isPacked=true.
  TemplateType->setBody(Types);

  // In order to handle global variable initializers that refer to other
  // global variables, this pass cannot delete any global variables yet.
  Constant *BasePtr = ConstantExpr::getIntToPtr(
      ConstantInt::get(IntPtrType, BaseAddr), TemplateType->getPointerTo());
  unsigned Index = 0;
  for (Module::global_iterator GV = M.global_begin(), E = M.global_end();
       GV != E; ++GV) {
    Constant *Indexes[] = {ConstantInt::get(I32, 0),
                           ConstantInt::get(I32, Index)};
    Value *Ptr = ConstantExpr::getGetElementPtr(BasePtr, Indexes);
    GV->replaceAllUsesWith(Ptr);
    Index++;
  }

  SmallVector<Constant *, 20> Fields;
  for (Module::global_iterator Iter = M.global_begin(), E = M.global_end();
       Iter != E;) {
    GlobalVariable *GV = Iter++;
    Fields.push_back(GV->getInitializer());
    GV->eraseFromParent();
  }
  Constant *Template = ConstantStruct::get(TemplateType, Fields);
  new GlobalVariable(M, TemplateType, /*isConstant=*/true,
                     GlobalVariable::ExternalLinkage, Template,
                     "__sfi_data_segment");

  DataLayout DL(&M);
  Constant *TemplateSize =
      ConstantInt::get(IntPtrType, DL.getTypeAllocSize(TemplateType));
  new GlobalVariable(M, TemplateSize->getType(), /*isConstant=*/true,
                     GlobalVariable::ExternalLinkage, TemplateSize,
                     "__sfi_data_segment_size");

  return true;
}

ModulePass *llvm::createAllocateDataSegmentPass() {
  return new AllocateDataSegment();
}
