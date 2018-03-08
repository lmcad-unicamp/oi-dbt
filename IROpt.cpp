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
#include "timer.hpp"


void dbt::IROpt::optimizeIRFunction(llvm::Module *M, OptLevel Level) {
  // Lazy initialization
  //
  
  //llvm::raw_ostream buffer;

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
    //  BasicPM->add(llvm::createLoopUnswitchPass());
      BasicPM->add(llvm::createInstructionCombiningPass());
      BasicPM->add(llvm::createIndVarSimplifyPass());       // Canonicalize indvars
     // BasicPM->add(llvm::createLoopDeletionPass());         // Delete dead loops
     // BasicPM->add(llvm::createLoopPredicationPass());
     // BasicPM->add(llvm::createSimpleLoopUnrollPass());     // Unroll small loops
      BasicPM->add(llvm::createCFGSimplificationPass());
      BasicPM->add(llvm::createInstructionCombiningPass());

      //Debug passes
      //BasicPM->add(llvm::createBitcodeWriterPass(llvm::outs()));
      //TM->addPassesToEmitFile(BasicPM, buffer.get(), TargetMachine::CGFT_ObjectFile, false);

      BasicPM->doInitialization();
    }

    for (auto &F : *M)
      BasicPM->run(F);

    //llvm::legacy::PassManager pPM;
    //auto PM = std::make_unique<llvm::legacy::PassManager>(M);

    //PM.add(new TargetLibraryInfoWrapperPass(Triple(TargetMachine::getTargetTriple())));
    // PM.add(llvm::createTargetTransformInfoWrapperPass(TargetMachine::getTargetIRAnalysis()));

    // std::unique_ptr<raw_fd_ostream> unopt_bc_OS;
    // std::unique_ptr<raw_fd_ostream> bc_OS;
    // std::unique_ptr<raw_fd_ostream> obj_OS;

    // int FD;
    // std::error_code EC = sys::fs::openFileForWrite("/home/napoli/nop1", FD, sys::fs::F_None);
    // unopt_bc_OS.reset(new raw_fd_ostream(FD, true));

    // if(EC)
    //   return -1;

  }

  //llvm::outs() << buffer << "\n";
}
