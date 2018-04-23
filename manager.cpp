#include <manager.hpp>
#include <OIPrinter.hpp>
#include <fstream>
#include <vector>
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
    spp::sparse_hash_map<uint32_t, uint32_t> BrTargets;

    if (getNumOfOIRegions() > 0) {
      OIRegionsMtx.lock_shared();
      EntryAddress = OIRegions.begin()->first;
      OIRegion     = OIRegions.begin()->second;
      BrTargets    = OIBrTargets[EntryAddress];
      OIRegionsMtx.unlock_shared();
    }

    if (OIRegion.size() == 0)
      continue;

    CompiledOIRegionsMtx.lock();
    CompiledOIRegions[EntryAddress] = OIRegion;
    CompiledOIRegionsMtx.unlock();

    //if (VerboseOutput)
      std::cerr << "Trying to compile: " << std::hex <<  EntryAddress  << "...";

    OICompiled += OIRegion.size();

    auto Module = IRE->generateRegionIR(EntryAddress, OIRegion, DataMemOffset, BrTargets, IRJIT->getTargetMachine(), NativeRegions);

    std::cerr << "OK" << std::endl;

/*    if (VerboseOutput)
      Module->print(llvm::errs(), nullptr);*/

    if (VerboseOutput) {
      std::cout << "---------------------- Printing OIRegion (OpenISA instr.) --------------------" << std::endl;

      for (auto Pair : OIRegion)
        std::cout << std::hex << Pair[0] << ":\t" << dbt::OIPrinter::getString(OIDecoder::decode(Pair[1])) << "\n";

      std::cout << "\n" << std::endl;
    }

    unsigned Size = 1;
    for (auto& F : *Module)
      for (auto& BB : F)
        Size += BB.size();

    IRO->optimizeIRFunction(Module, IROpt::OptLevel::Basic);

    unsigned OSize = 1;
    for (auto& F : *Module)
      for (auto& BB : F)
        OSize += BB.size();

    if (VerboseOutput)
      Module->print(llvm::errs(), nullptr);

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

    NativeRegionsMtx.unlock();

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

  if(isRegionRecorging)
    RegionAddresses.clear();

  while (isNativeRegionEntry(JumpTo)) {
    uint32_t LastTo = JumpTo;

    if(isRegionRecorging)
      RegionAddresses.push_back(LastTo);

    //std::cerr << "\nStarting native execution on Address: " << std::hex << JumpTo << std::endl;
    //std::cerr << "State before entering region...";
    //M.dumpRegisters();

    /*auto Addr = IRJIT->findSymbol("r"+std::to_string(JumpTo)).getAddress();
    std::cerr << "Disassembly of Region: " << JumpTo << ":" << std::endl;
    std::ostringstream buffer;
    size_t t = IREmitter::disassemble((const void*) *Addr, buffer);
    std::cerr << buffer.str().c_str() << std::endl;*/


    M.setOnNativeExecution(JumpTo);
    uint32_t (*FP)(int32_t*, uint32_t*, volatile uint64_t*) = (uint32_t (*)(int32_t*, uint32_t*, volatile uint64_t*)) NativeRegions[JumpTo];
    //assert(FP != NULL && "Error... Not compiled!");
    JumpTo = FP(M.getRegisterPtr(), M.getMemoryPtr(), NativeRegions);

    //std::cerr << "Back to interpreter! Ret Address: " << std::hex << JumpTo << std::endl;
    //std::cerr << "State after exiting region...\n";
    //M.dumpRegisters();
    //M.setPC(JumpTo);
    //break;
  }

  M.setOffNativeExecution();

  //std::cerr << "Ending native execution. Ret Address: " << std::hex << JumpTo << std::endl;

  return JumpTo;
}
