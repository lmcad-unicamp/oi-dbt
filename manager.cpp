#include <manager.hpp>
#include <OIPrinter.hpp>
#include <fstream>
#include <vector>
#include "llvm/IR/Verifier.h"
#include "llvm/Support/raw_ostream.h"
#include "timer.hpp"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/IRReader/IRReader.h"

using namespace dbt;

llvm::Module* Manager::loadRegionFromFile(std::string Path) {
  llvm::SMDiagnostic error;
  auto M = llvm::parseIRFile(Path, error, TheContext).release();
  if (M) {
     return M;
  } else {
    //std::cerr << Path << " " << error.getMessage().str() << "\n";
    return nullptr;
  }
}

void Manager::loadRegionsFromFiles() {
  std::ifstream infile("regions.order");
  std::string line;
  OIRegionsMtx.lock();
  while (std::getline(infile, line)) {
    uint32_t Entry = std::stoi(line);
    OIRegionsKey.push_back(Entry);
    if (!IsToLoadBCFormat) {
      std::ifstream infile("r"+std::to_string(Entry)+".oi"); 
      std::string line;
      while (std::getline(infile, line)) {
        std::istringstream iss(line);
        uint32_t Addrs, Opcode;
        if (!(iss >> Addrs >> Opcode)) { break; }
        OIRegions[Entry].push_back({Addrs, Opcode});
      }
    } else {
      OIRegions[Entry] = {{Entry,0}};
    }
  }
  OIRegionsMtx.unlock();
}

void Manager::runPipeline() {
  if (!IRE) {
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();
    IRJIT = new llvm::orc::IRLazyJIT();
  }

  IRE = llvm::make_unique<IREmitter>();
  IRO = llvm::make_unique<IROpt>();

  if (IsToLoadRegions)
    loadRegionsFromFiles();

  while (isRunning) {
    uint32_t EntryAddress;
    OIInstList OIRegion;

    if (getNumOfOIRegions() > 0) {
      OIRegionsMtx.lock_shared();
      EntryAddress = OIRegionsKey.front();
      OIRegion     = OIRegions[EntryAddress];
      OIRegionsMtx.unlock_shared();
    }

    llvm::Module* Module = nullptr;
    unsigned Size = 1;
    unsigned OSize = 1;

    if (OIRegion.size() == 0) continue;

    if (IsToLoadRegions && IsToLoadBCFormat) 
      Module = loadRegionFromFile("r"+std::to_string(EntryAddress)+".bc");

    if (Module == nullptr) {
      CompiledOIRegionsMtx.lock();
      CompiledOIRegions[EntryAddress] = OIRegion;
      CompiledOIRegionsMtx.unlock();

      if (VerboseOutput)
        std::cerr << "Trying to compile: " << std::hex <<  EntryAddress  << "...";

      OICompiled += OIRegion.size();

      Module = IRE->generateRegionIR(EntryAddress, OIRegion, DataMemOffset, TheMachine, IRJIT->getTargetMachine(), NativeRegions);
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

      for (auto& F : *Module)
        for (auto& BB : F)
          Size += BB.size();

      if (OptMode != OptPolitic::Custom) 
        IRO->optimizeIRFunction(Module, IROpt::OptLevel::Basic);
      else if (CustomOpts->count(EntryAddress) != 0) 
        IRO->customOptimizeIRFunction(Module, (*CustomOpts)[EntryAddress]);

      for (auto& F : *Module)
        for (auto& BB : F)
          OSize += BB.size();
    }

    // Remove a region if the first instruction is a return <- can cause infinity loops
    llvm::Function* LLVMRegion = Module->getFunction("r"+std::to_string(EntryAddress));
    auto Inst     = LLVMRegion->getEntryBlock().getFirstNonPHI();
    bool IsRet    = Inst->getOpcode() == llvm::Instruction::Ret;
    bool RetLoop = true;
    if (IsRet) {
      auto RetInst = llvm::dyn_cast<llvm::ReturnInst>(Inst);
      if (llvm::isa<llvm::ConstantInt>(RetInst->getReturnValue())) 
        if (!llvm::dyn_cast<llvm::ConstantInt>(RetInst->getReturnValue())->equalsInt(EntryAddress)) 
          RetLoop = false;
    }

    if (!IsRet || !RetLoop) {
      if (VerboseOutput)
        Module->print(llvm::errs(), nullptr);

      IRRegions[EntryAddress] = llvm::CloneModule(Module).release();
      IRRegionsKey.push_back(EntryAddress);

      NativeRegionsMtx.lock();
      IRJIT->addModule(std::unique_ptr<llvm::Module>(Module));

      if (VerboseOutput)
        llvm::errs() << ".. we've compiled (" << (float) OSize/Size << ")\n";

      CompiledRegions += 1;
      LLVMCompiled += OSize;
      AvgOptCodeSize += (float) OSize/Size;

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
    OIRegionsKey.erase(OIRegionsKey.begin());
    OIRegionsMtx.unlock();
  }
  isFinished = true;
}

bool Manager::addOIRegion(uint32_t EntryAddress, OIInstList OIRegion) {
  if (!isRegionEntry(EntryAddress) && OIRegions.count(EntryAddress) == 0) {
    OIRegionsMtx.lock();
    OIRegionsKey.push_back(EntryAddress);
    OIRegions[EntryAddress]   = OIRegion;
    OIRegionsMtx.unlock();
    return true;
  }
  return false;
}

int32_t Manager::jumpToRegion(uint32_t EntryAddress) {
  uint32_t JumpTo = EntryAddress;
  uint32_t LastTo = JumpTo;
  int32_t* RegPtr = TheMachine.getRegisterPtr();
  uint32_t* MemPtr = TheMachine.getMemoryPtr();

  while (isNativeRegionEntry(JumpTo)) {
    LastTo = JumpTo;
    uint32_t (*FP)(int32_t*, uint32_t*) = (uint32_t (*)(int32_t*, uint32_t*)) NativeRegions[JumpTo];
    JumpTo = FP(RegPtr, MemPtr);
  }

  return JumpTo;
}
