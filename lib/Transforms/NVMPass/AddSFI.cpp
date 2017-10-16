#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
// #include "llvm/IR/Intrinsics.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/NVMPass.h"

using namespace llvm;

namespace {
// This is a ModulePass so that it can add global variables.
class ExpandAllocas : public ModulePass {
public:
  static char ID; // Pass identification, replacement for typeid
  ExpandAllocas() : ModulePass(ID) {
    initializeExpandAllocasPass(*PassRegistry::getPassRegistry());
  }

  virtual bool runOnModule(Module &M);
};
} // namespace

char ExpandAllocas::ID = 0;
INITIALIZE_PASS(ExpandAllocas, "expand-allocas",
                "Expand out alloca instructions", false, false)

static void expandAllocas(Function *Func, Type *IntPtrType, Value *StackPtr) {
  // Skip function declarations.
  if (Func->empty())
    return;

  Type *I8Ptr = Type::getInt8PtrTy(Func->getContext());
  Instruction *FrameTop = NULL;

  BasicBlock *EntryBB = &Func->getEntryBlock();
  unsigned FrameOffset = 0;
  for (BasicBlock::iterator Iter = EntryBB->begin(), E = EntryBB->end();
       Iter != E; Iter++) {
    Instruction *Inst = &(*Iter);
    if (AllocaInst *Alloca = dyn_cast<AllocaInst>(Inst)) {
      // XXX: error reporting
      assert(Alloca->getType() == I8Ptr);
      // XXX: error reporting
      ConstantInt *CI = cast<ConstantInt>(Alloca->getArraySize());
      // TODO: handle alignment
      FrameOffset += CI->getZExtValue();

      if (!FrameTop) {
        FrameTop = new LoadInst(StackPtr, "frame_top");
        EntryBB->getInstList().push_front(FrameTop);
      }
      Value *Var = BinaryOperator::Create(
          BinaryOperator::Add, FrameTop,
          ConstantInt::get(IntPtrType, -FrameOffset), "", Alloca);
      Var = new IntToPtrInst(Var, Alloca->getType(), "", Alloca);
      Var->takeName(Alloca);
      Alloca->replaceAllUsesWith(Var);
      Alloca->eraseFromParent();
    }
  }
  if (FrameTop) {
    // Adjust stack pointer.
    // TODO: Could omit this in leaf functions.
    Instruction *FrameBottom = BinaryOperator::Create(
        BinaryOperator::Add, FrameTop,
        ConstantInt::get(IntPtrType, -FrameOffset), "frame_bottom");
    FrameBottom->insertAfter(FrameTop);
    (new StoreInst(FrameBottom, StackPtr))->insertAfter(FrameBottom);

    for (Function::iterator BB = Func->begin(), E = Func->end(); BB != E;
         ++BB) {
      for (BasicBlock::iterator Inst = BB->begin(), E = BB->end(); Inst != E;
           ++Inst) {
        if (isa<AllocaInst>(Inst)) {
          report_fatal_error("TODO: handle dynamic alloca");
        } else if (ReturnInst *Ret = dyn_cast<ReturnInst>(Inst)) {
          // Restore stack pointer.
          new StoreInst(FrameTop, StackPtr, Ret);
        }
      }
    }
  }
}

bool ExpandAllocas::runOnModule(Module &M) {
  Type *IntPtrType = Type::getInt32Ty(M.getContext()); // XXX
  uint64_t InitialStackPtr = 0x40000000;
  Value *StackPtr = new GlobalVariable(
      M, IntPtrType, /*isConstant=*/false, GlobalVariable::InternalLinkage,
      ConstantInt::get(IntPtrType, InitialStackPtr), "__sfi_stack");

  for (Module::iterator Func = M.begin(), E = M.end(); Func != E; ++Func) {
    expandAllocas(&(*Func), IntPtrType, StackPtr);
  }
  return true;
}

ModulePass *llvm::createExpandAllocasPass() { return new ExpandAllocas(); }
