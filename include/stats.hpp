#ifndef STATS_HPP
#define STATS_HPP

#include <OIDecoder.hpp>
#include <syscall.hpp>
#include <IREmitter.hpp>
#include <map>

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/Type.h"
#include "llvm/ExecutionEngine/MCJIT.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm-c/Core.h"
#include "llvm-c/Disassembler.h"
#include "polly/CodeGen/IslAst.h"
#include "polly/CodeGen/IslNodeBuilder.h"
#include "polly/CodeGen/PerfMonitor.h"
#include "polly/CodeGen/Utils.h"
#include "polly/DependenceInfo.h"
#include "polly/LinkAllPasses.h"
#include "polly/Options.h"
#include "polly/ScopInfo.h"
#include "polly/Support/ScopHelper.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/BasicAliasAnalysis.h"
#include "llvm/Analysis/GlobalsModRef.h"
#include "llvm/Analysis/ScalarEvolutionAliasAnalysis.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Debug.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Type.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm-c/Core.h"
#include "llvm-c/Disassembler.h"
#include "llvm-c/Target.h"

using namespace llvm;
using namespace polly;

class StatsMonitor
{
private:
  bool Supported = false;
  Module* mod;
  Function* func;
  std::unique_ptr<llvm::IRBuilder<>> &Builder;
  llvm::Value *CyclesTotalStartPtr = nullptr, *CyclesEndPtr = nullptr, *RDTSCPWriteLocation = nullptr, AlreadyInitializedPtr = nullptr;

public:
  StatsMonitor(Module* M, Function* F, std::unique_ptr<llvm::IRBuilder<>>* B;) : mod(M), func(F), Builder(B),  {
    if (Triple(mod->getTargetTriple()).getArch() == llvm::Triple::x86_64)
      Supported = true;
   else
      Supported = false;

  };

  int attachMonitor(void);

};



#endif /* STATS_HPP */
