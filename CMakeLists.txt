cmake_minimum_required(VERSION 3.5)
project(openisa-dbt)

set(CMAKE_CXX_FLAGS "-O3 -Werror -Wall -Wextra -Wno-unused-function -std=c++17 -fno-rtti -flto -DNDEBUG")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -O3 -flto")

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

  LLVMPasses
  LLVMObjCARCOpts
  LLVMSymbolize
  LLVMDebugInfoPDB
  LLVMDebugInfoDWARF
  LLVMCoverage
  LLVMTableGen
  LLVMLineEditor
  LLVMOrcJIT
  LLVMMIRParser
  LLVMObjectYAML
  LLVMLibDriver
  LLVMOption
  LLVMX86Disassembler
  LLVMX86AsmParser
  LLVMX86CodeGen
  LLVMGlobalISel
  LLVMSelectionDAG
  LLVMAsmPrinter
  LLVMDebugInfoCodeView
  LLVMDebugInfoMSF
  LLVMX86Desc
  LLVMMCDisassembler
  LLVMX86Info
  LLVMX86AsmPrinter
  LLVMX86Utils
  LLVMMCJIT
  LLVMMCJIT
  LLVMInterpreter
  LLVMExecutionEngine
  LLVMRuntimeDyld
  LLVMCodeGen
  LLVMTarget
  LLVMCoroutines
  LLVMipo
  LLVMInstrumentation
  LLVMVectorize
  LLVMScalarOpts
  LLVMLinker
  LLVMIRReader
  LLVMAsmParser
  LLVMInstCombine
  LLVMTransformUtils
  LLVMBitWriter
  LLVMTransformUtils
  LLVMAnalysis
  LLVMTarget
  LLVMObject
  LLVMMCParser
  LLVMBitReader
  LLVMMC

  LLVMCore
  LLVMSupport
  LLVMDemangle
)
