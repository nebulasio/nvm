
add_custom_target(nvmtest)
message(STATUS ${LLVM_BINARY_DIR})
add_custom_target(sample_test
  COMMAND clang -S -emit-llvm ${PROJECT_SOURCE_DIR}/nvmtests/sample.c
  COMMAND nebulas-vm -assembly=${LLVM_BINARY_DIR}/bin/sample.ll -signature=xx -token=xx
  WORKING_DIRECTORY ${LLVM_BINARY_DIR}/bin
  )
add_dependencies(nvmtest sample_test)
