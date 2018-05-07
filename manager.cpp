#include <manager.hpp>
#include <OIPrinter.hpp>
#include <fstream>
#include <vector>
#include "llvm/IR/Verifier.h"
#include "llvm/Support/raw_ostream.h"
#include "timer.hpp"
#include "llvm/Transforms/Utils/Cloning.h"

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
    spp::sparse_hash_map<uint32_t, uint32_t> BrTargets;

    if (getNumOfOIRegions() > 0) {
      OIRegionsMtx.lock_shared();
      EntryAddress = OIRegions.begin()->first;
      OIRegion     = OIRegions.begin()->second;
      BrTargets    = OIBrTargets[EntryAddress];
      OIRegionsMtx.unlock_shared();
    }

    if (OIRegion.size() == 0) continue;

    CompiledOIRegionsMtx.lock();
    CompiledOIRegions[EntryAddress] = OIRegion;
    CompiledOIRegionsMtx.unlock();

    if (VerboseOutput)
      std::cerr << "Trying to compile: " << std::hex <<  EntryAddress  << "...";

    OICompiled += OIRegion.size();

    auto Module = IRE->generateRegionIR(EntryAddress, OIRegion, DataMemOffset, BrTargets, IRJIT->getTargetMachine(), NativeRegions);
    if (VerboseOutput)
      std::cerr << "OK" << std::endl;

    if (VerboseOutput)
      Module->print(llvm::errs(), nullptr);

    if (VerboseOutput) {
      std::cerr << "---------------------- Printing OIRegion (OpenISA instr.) --------------------" << std::endl;

      for (auto Pair : OIRegion)
        std::cerr << std::hex << Pair[0] << ":\t" << dbt::OIPrinter::getString(OIDecoder::decode(Pair[1])) << "\n";

      std::cerr << "\n" << std::endl;
    }

    unsigned Size = 1;
    for (auto& F : *Module)
      for (auto& BB : F)
        Size += BB.size();

    if (OptMode != OptPolitic::Custom) {
      IRO->optimizeIRFunction(Module, IROpt::OptLevel::Basic);
    } else if (CustomOpts->count(EntryAddress) != 0) {
      IRO->customOptimizeIRFunction(Module, (*CustomOpts)[EntryAddress]);
    }

    unsigned OSize = 1;
    for (auto& F : *Module)
      for (auto& BB : F)
        OSize += BB.size();

    // Remove a region if the first instruction is a return <- can cause infinity loops
    llvm::Function* LLVMRegion = Module->getFunction("r"+std::to_string(EntryAddress));
    if (LLVMRegion->getEntryBlock().getFirstNonPHI()->getOpcode() != llvm::Instruction::Ret) {
      if (VerboseOutput)
        Module->print(llvm::errs(), nullptr);

      IRRegions[EntryAddress] = llvm::CloneModule(Module).release();

      IRJIT->addModule(std::unique_ptr<llvm::Module>(Module));

      if (VerboseOutput)
        llvm::errs() << ".. we've compiled (" << (float) OSize/Size << ")\n";

      CompiledRegions += 1;
      LLVMCompiled += OSize;
      AvgOptCodeSize += (float) OSize/Size;

      NativeRegionsMtx.lock();

      auto Addr = IRJIT->findSymbol("r"+std::to_string(EntryAddress)).getAddress();

      if (Addr)
        NativeRegions[EntryAddress] = static_cast<intptr_t>(*Addr);
      else
        std::cerr << EntryAddress << " was not successfully compiled!\n";

      NativeRegionsMtx.unlock();

      if (VerboseOutput) {
        std::cerr << "Disassembly of Region: " << EntryAddress << ":" << std::endl;
        std::ostringstream buffer;
        size_t t = IREmitter::disassemble((const void*) *Addr, buffer);
        std::cerr << buffer.str().c_str() << std::endl;

        buffer.clear();
        buffer.str("");

        std::cerr << "Dumping Region: " << EntryAddress << ":" << std::endl;
        IREmitter::regionDump((const void*) *Addr, buffer, t);
        std::cerr << buffer.str().c_str() << std::endl;
      }

    } else if (VerboseOutput) {
        std::cerr << "Giving up " << std::hex << EntryAddress << " compilation as it starts with a return!\n";
    }

    OIRegionsMtx.lock();
    OIRegions.erase(EntryAddress);
    OIRegionsMtx.unlock();
  }
  isFinished = true;
}

bool Manager::addOIRegion(uint32_t EntryAddress, OIInstList OIRegion, spp::sparse_hash_map<uint32_t, uint32_t> BrTargets) {
  if (!isRegionEntry(EntryAddress) && OIRegion.size() > 3) {
    OIRegionsMtx.lock();
    OIRegions[EntryAddress]   = OIRegion;
    OIBrTargets[EntryAddress] = BrTargets;
    OIRegionsMtx.unlock();
    return true;
  }
  return false;
}

int32_t Manager::jumpToRegion(uint32_t EntryAddress, dbt::Machine& M) {
  uint32_t JumpTo = EntryAddress;

  while (isNativeRegionEntry(JumpTo)) {
    uint32_t LastTo = JumpTo;

    uint32_t (*FP)(int32_t*, uint32_t*, volatile uint64_t*) = (uint32_t (*)(int32_t*, uint32_t*, volatile uint64_t*)) NativeRegions[JumpTo];
    JumpTo = FP(M.getRegisterPtr(), M.getMemoryPtr(), NativeRegions);
  }

  return JumpTo;
}
