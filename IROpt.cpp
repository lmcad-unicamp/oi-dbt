#include <IROpt.hpp>

// Opt
#include "llvm/Analysis/Passes.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"

void dbt::IROpt::optimizeIRFunction(llvm::Module* M, OptLevel Level) {
  // Lazy initialization
  //
  if (Level == OptLevel::Basic) {
    auto BasicPM = std::make_unique<llvm::legacy::FunctionPassManager>(M);
    
    // Promote allocas to registers.
    BasicPM->add(llvm::createPromoteMemoryToRegisterPass());
    // Do simple "peephole" optimizations and bit-twiddling optzns.
    BasicPM->add(llvm::createInstructionCombiningPass());
    // Reassociate expressions.
    BasicPM->add(llvm::createReassociatePass());
    // Eliminate Common SubExpressions.
    BasicPM->add(llvm::createGVNPass());
    // Loop Code Motion
    BasicPM->add(llvm::createLICMPass());

    for (auto& F : *M)
      BasicPM->run(F);
  }
}
