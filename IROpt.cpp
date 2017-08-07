#include <IROpt.hpp>

// Opt
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"

void dbt::IROpt::optimizeIRFunction(llvm::Module *M, OptLevel Level) {
  // Lazy initialization
  //
  if (Level == OptLevel::Basic) {
    if (!BasicPM) {
      BasicPM = std::make_unique<llvm::legacy::FunctionPassManager>(M);

      BasicPM->add(llvm::createInstructionCombiningPass());
      BasicPM->add(llvm::createReassociatePass());
      BasicPM->add(llvm::createNewGVNPass());
      BasicPM->add(llvm::createCFGSimplificationPass());
      BasicPM->add(llvm::createLICMPass());

      BasicPM->doInitialization();
    }

    for (auto &F : *M)
      BasicPM->run(F);
  }
}
