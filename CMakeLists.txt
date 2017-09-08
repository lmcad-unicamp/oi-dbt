cmake_minimum_required(VERSION 3.5)
project(openisa-dbt)

set(CMAKE_CXX_FLAGS "-O3 -Werror -Wall -Wextra -Wno-unused-function -std=c++17 -fno-rtti -flto -DNDEBUG -L/usr/local/lib/ -I/usr/local/include -fPIC -fvisibility-inlines-hidden -ffunction-sections -fdata-sections -D_GNU_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -O3 -flto -fno-rtti")

include_directories(include)

add_subdirectory(RFT)
add_subdirectory(arglib)

add_library(dbt IREmitter.cpp IRUtils.cpp IROpt.cpp interpreter.cpp machine.cpp manager.cpp syscall.cpp)

add_executable(oi-dbt main.cpp)

install(TARGETS oi-dbt RUNTIME DESTINATION bin)

target_link_libraries(oi-dbt
  papi 
  RFT 
  dbt 
  arglib 
  pthread
  z
  dl
  curses

  LLVMLTO LLVMPasses LLVMObjCARCOpts LLVMSymbolize LLVMDebugInfoPDB LLVMDebugInfoDWARF LLVMFuzzMutate LLVMTableGen 
  LLVMDlltoolDriver LLVMLineEditor LLVMOrcJIT LLVMCoverage LLVMMIRParser LLVMObjectYAML LLVMLibDriver LLVMOption 
  LLVMWindowsManifest LLVMX86Disassembler LLVMX86AsmParser LLVMX86CodeGen LLVMGlobalISel LLVMSelectionDAG 
  LLVMAsmPrinter LLVMDebugInfoCodeView LLVMDebugInfoMSF LLVMX86Desc LLVMMCDisassembler LLVMX86Info 
  LLVMX86AsmPrinter LLVMX86Utils LLVMMCJIT LLVMInterpreter LLVMExecutionEngine LLVMRuntimeDyld LLVMCodeGen 
  LLVMTarget LLVMCoroutines LLVMipo LLVMInstrumentation LLVMVectorize LLVMScalarOpts LLVMLinker LLVMIRReader 
  LLVMAsmParser LLVMInstCombine LLVMTransformUtils LLVMBitWriter LLVMAnalysis LLVMProfileData LLVMObject LLVMMCParser 
  LLVMMC LLVMBitReader LLVMCore LLVMBinaryFormat LLVMSupport LLVMDemangle
)
