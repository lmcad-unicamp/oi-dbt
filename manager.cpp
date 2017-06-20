#include <manager.hpp>
#include "llvm/IR/Verifier.h"
#include "llvm/Support/raw_ostream.h"
#include "timer.hpp"

using namespace dbt;

void Manager::runPipeline() {
  if (!IRE) {
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

    IRE = llvm::make_unique<IREmitter>();
    IRO = llvm::make_unique<IROpt>();
    IRJIT = new llvm::orc::IRLazyJIT();
  }

  while (isRunning) { 
    uint32_t EntryAddress;
    OIInstList OIRegion;

    if (getNumOfOIRegions() > 0) {
      OIRegionsMtx.lock_shared();
      EntryAddress = OIRegions.begin()->first;
      OIRegion = OIRegions.begin()->second;
      OIRegionsMtx.unlock_shared();
    } 

    if (OIRegion.size() == 0) { 
      continue;
    }

    CompiledOIRegionsMtx.lock();
    CompiledOIRegions[EntryAddress] = OIRegion; 
    CompiledOIRegionsMtx.unlock();

    OICompiled += OIRegion.size();
    auto Module = IRE->generateRegionIR(EntryAddress, OIRegion, DataMemOffset); 

    unsigned Size = 0;
    for (auto& F : *Module) 
      for (auto& BB : F)
        Size += BB.size(); 

    IRO->optimizeIRFunction(Module, IROpt::OptLevel::Basic); 

    unsigned OSize = 1;
    for (auto& F : *Module) 
      for (auto& BB : F)
        OSize += BB.size(); 

/*    for (auto& F : *Module) 
      F.print(llvm::errs());*/

    IRJIT->addModule(std::unique_ptr<llvm::Module>(Module));

    llvm::errs() << "We've compiled: " <<  EntryAddress << " " << (float) OSize/Size << "\n";
    CompiledRegions += 1;
    LLVMCompiled += OSize;
    AvgOptCodeSize += (float) OSize/Size;

    NativeRegionsMtx.lock();
    NativeRegions[EntryAddress] = (intptr_t) IRJIT->findSymbol("r"+std::to_string(EntryAddress)).getAddress();
    NativeRegionsMtx.unlock();

    OIRegionsMtx.lock();
    OIRegions.erase(EntryAddress);
    OIRegionsMtx.unlock();
  }
}

void Manager::addOIRegion(uint32_t EntryAddress, OIInstList OIRegion) {
  if (!isRegionEntry(EntryAddress)) {
    OIRegionsMtx.lock();
    OIRegions[EntryAddress] = OIRegion;
    OIRegionsMtx.unlock();
  }
}

int32_t Manager::jumpToRegion(uint32_t EntryAddress, dbt::Machine& M) {
  uint32_t JumpTo = EntryAddress;
  while (isNativeRegionEntry(JumpTo)) {
    uint32_t (*FP)(int32_t*, uint32_t*) = (uint32_t (*)(int32_t*, uint32_t*)) NativeRegions[JumpTo];
    JumpTo = FP(M.getRegisterPtr(), M.getMemoryPtr());
  }
  return JumpTo;
}
