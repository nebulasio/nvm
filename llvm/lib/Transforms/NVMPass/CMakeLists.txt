add_llvm_library(LLVMNVMPass
  AddSFI.cpp
  SandboxIndirectCalls.cpp
  SandboxMemoryAccess.cpp
  StripTLS.cpp

  ADDITIONAL_HEADER_DIRS
  ${LLVM_MAIN_INCLUDE_DIR}/llvm/Transforms

  DEPENDS
  intrinsics_gen
  )
