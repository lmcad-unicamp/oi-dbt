#include <IROpt.hpp>

// Opt
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"

void dbt::IROpt::optimizeIRFunction(llvm::Module *M, OptLevel Level) {
  // Lazy initialization
  //
  if (Level == OptLevel::Basic) {
    auto BasicPM = std::make_unique<llvm::legacy::FunctionPassManager>(M);

    //BasicPM->add(llvm::createInstructionCombiningPass());
    BasicPM->add(llvm::createGVNPass());
    //BasicPM->add(llvm::createDSEPass());
    BasicPM->add(llvm::createLICMPass());
    //BasicPM->add(llvm::createInstructionCombiningPass());
    BasicPM->add(llvm::createGVNPass());

    for (auto &F : *M)
      BasicPM->run(F);
  }
}
