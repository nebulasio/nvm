
#include "engine.h"
#include "runtime/nebulas.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/raw_ostream.h"
#include <algorithm>
#include <iostream>
#include <string>
#include <system_error>

using namespace llvm;

cl::opt<std::string> AssemblyFilePath("assembly", cl::desc("The LLVM IR file"),
                                      cl::Required);
cl::opt<std::string> AssemblyFileSignature(
    "signature", cl::desc("The signature of the LLVM IR file"), cl::Required);

cl::opt<std::string> ExecutionToken("token",
                                    cl::desc("Token to authorize execution"),
                                    cl::Required);
enum ExeLevel { g, O1, O2, O3 };

cl::opt<ExeLevel> OptimizationLevel(
    cl::desc("Choose execution level:"),
    cl::values(clEnumVal(g, "No optimizations, enable debugging"),
               clEnumVal(O1, "Enable trivial optimizations"),
               clEnumVal(O2, "Enable default optimizations"),
               clEnumVal(O3, "Enable expensive optimizations")));

int main(int argc, const char *argv[]) {
  // Print a stack trace if we signal out.
  sys::PrintStackTraceOnErrorSignal(argv[0]);
  PrettyStackTraceProgram X(argc, argv);

  // Hide options from LLVM
  StringMap<cl::Option *> &Map = cl::getRegisteredOptions();
  std::vector<std::string> hideOptions = {"x86-asm-syntax",
                                          "verify-scev-maps",
                                          "verify-scev",
                                          "verify-regalloc",
                                          "verify-machine-dom-info",
                                          "verify-loop-lcssa",
                                          "verify-loop-info",
                                          "verify-dom-info",
                                          "verify-debug-info",
                                          "time-passes",
                                          "stats-json",
                                          "stats",
                                          "simplify-mir",
                                          "rng-seed",
                                          "reverse-iterate",
                                          "regalloc",
                                          "disable-spill-fusing",
                                          "enable-implicit-null-checks",
                                          "enable-load-pre",
                                          "enable-objc-arc-opts",
                                          "enable-scoped-noalias",
                                          "exhaustive-register-search",
                                          "expensive-combines",
                                          "imp-null-check-page-size",
                                          "imp-null-max-insts-to-consider",
                                          "instcombine-maxarray-size",
                                          "join-liveintervals",
                                          "limit-float-precision",
                                          "rewrite-map-file",
                                          "stackmap-version"};

  for (size_t i = 0; i < hideOptions.size(); ++i) {
    const std::string &option = hideOptions[i];
    if (Map.count(option) > 0) {
      Map[option]->setHiddenFlag(cl::ReallyHidden);
    } else {
      std::cout << "unset option " << option << ", ignore." << std::endl;
    }
  }
  // llvm_shutdown_obj Y; // Call llvm_shutdown() on exit.
  cl::ParseCommandLineOptions(argc, argv, "Nebulas Virtual Machine (NVM)\n");

  nebulas_code_t status;
  status = check_priviliege(ExecutionToken.c_str());
  if (status != code_succ) {
    std::cout << "No priviliege to run Nebulas VM." << std::endl;
    return status;
  }
  status =
      check_assembly(AssemblyFilePath.c_str(), AssemblyFileSignature.c_str());
  if (status != code_succ) {
    std::cout << "Invalid assembly file, not exist or wrong signature."
              << std::endl;
    return status;
  }

  // TODO, we should use some better log lib, like glog here
  Initialize();
  printf("initialized.\n");

  Engine *e = CreateEngine();
  printf("engine created.\n");

  AddModuleFile(e, AssemblyFilePath.c_str());
  printf("added Module file %s\n", AssemblyFilePath.c_str());

  int ret = RunFunction(e, "nebulas_main", 0, NULL);
  printf("runFunction return %d\n", ret);

  DeleteEngine(e);
  printf("engine deleted.\n");

  return code_succ;
}
