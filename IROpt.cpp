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
    
    // Do simple "peephole" optimizations and bit-twiddling optzns.
    BasicPM->add(llvm::createInstructionCombiningPass());
    // Eliminate Common SubExpressions.
    BasicPM->add(llvm::createGVNPass());
    // Loop Code Motion
    BasicPM->add(llvm::createLICMPass());

    for (auto& F : *M)
      BasicPM->run(F);
  }
}
