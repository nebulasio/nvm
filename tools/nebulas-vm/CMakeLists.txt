#TODO we just put all components here, later we can remove unnecessary components
set(LLVM_LINK_COMPONENTS
  ${LLVM_TARGETS_TO_BUILD}
  Analysis
  BitWriter
  CodeGen
  Core
  Coroutines
  IPO
  IRReader
  InstCombine
  Instrumentation
  MC
  ObjCARCOpts
  ScalarOpts
  Support
  Target
  TransformUtils
  Vectorize
  Passes
  AsmPrinter
  AsmParser
  MCJIT
  ExecutionEngine
  NVMPass
  )


add_llvm_tool(nebulas-vm
  engine.cpp
  memory_manager.cpp
  nebulas_vm.cpp
  checker/global_variable.cpp
  runtime/nebulas.cpp
  )
