#include <IROpt.hpp>
#include <manager.hpp>
#include <OIPrinter.hpp>

// Opt
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/Bitcode/BitcodeWriterPass.h"

#include "llvm/IR/LegacyPassManagers.h"
#include "llvm/IR/IRPrintingPasses.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "timer.hpp"

void dbt::IROpt::optimizeIRFunction(llvm::Module *M, OptLevel Level) {
  // Lazy initialization
  if (Level == OptLevel::Basic) {
    if (!BasicPM) {
      BasicPM = std::make_unique<llvm::legacy::FunctionPassManager>(M);

      BasicPM->add(llvm::createInstructionCombiningPass());
      BasicPM->add(llvm::createCFGSimplificationPass());
      BasicPM->add(llvm::createReassociatePass());
      BasicPM->add(llvm::createNewGVNPass());
      BasicPM->add(llvm::createDeadInstEliminationPass());
      BasicPM->add(llvm::createDeadCodeEliminationPass());
      BasicPM->add(llvm::createPromoteMemoryToRegisterPass());
      BasicPM->add(llvm::createInstructionCombiningPass());
      BasicPM->add(llvm::createLICMPass());
      BasicPM->add(llvm::createMemCpyOptPass());
      BasicPM->add(llvm::createLoopUnswitchPass());
      BasicPM->add(llvm::createInstructionCombiningPass());
      BasicPM->add(llvm::createIndVarSimplifyPass());       // Canonicalize indvars
      BasicPM->add(llvm::createLoopDeletionPass());         // Delete dead loops
      BasicPM->add(llvm::createLoopPredicationPass());
      BasicPM->add(llvm::createSimpleLoopUnrollPass());     // Unroll small loops
      BasicPM->add(llvm::createCFGSimplificationPass());
      BasicPM->add(llvm::createInstructionCombiningPass());

/*      llvm::PassManagerBuilder Builder;
      Builder.OptLevel = 3;
      Builder.SizeLevel = 0;*/

//      Builder.populateFunctionPassManager(*BasicPM.get());

      BasicPM->doInitialization();
    }

    for (auto &F : *M)
      BasicPM->run(F);
  }
}
