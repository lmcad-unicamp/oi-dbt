#include <manager.hpp>
#include <OIPrinter.hpp>
#include <fstream>
#include <vector>
#include "llvm/IR/Verifier.h"
#include "llvm/Support/raw_ostream.h"
#include "timer.hpp"
#include "llvm/Transforms/Utils/Cloning.h"
#include <experimental/filesystem>
#include "llvm/Support/SourceMgr.h"
#include "llvm/IRReader/IRReader.h"

namespace fs = std::experimental::filesystem;
using namespace dbt;

void Manager::loadRegionsFromFiles() {
	llvm::SMDiagnostic error;
	for(const auto& p : fs::directory_iterator("./")) {
		if(p.path().extension() == ".bc") { 
			std::string Path = p.path().string();
			Path.erase(0,3);
			Path.erase(Path.size()-3, 3);
			auto M = llvm::parseIRFile(p.path().string(), error, TheContext).release();
			if (M) {
				ModulesLoaded[std::stoi(Path)] = M;
			} else {
			 	std::cerr << p.path() << " " << error.getMessage().str() << "\n";
			}
		}
	}
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
		spp::sparse_hash_map<uint32_t, uint32_t> BrTargets;

		if (getNumOfOIRegions() > 0) {
			OIRegionsMtx.lock_shared();
			EntryAddress = OIRegionsKey.front();
			OIRegion     = OIRegions[EntryAddress];
			BrTargets    = OIBrTargets[EntryAddress];
			OIRegionsMtx.unlock_shared();
		}

		llvm::Module* Module;
		unsigned Size = 1;
		unsigned OSize = 1;

		if (IsToLoadRegions && ModulesLoaded.count(EntryAddress) != 0) {
			//std::cerr << "loading" << EntryAddress << "\n";
			Module = ModulesLoaded[EntryAddress]; 
			ModulesLoaded.erase(EntryAddress);
		} else {
			//std::cerr << "compiling" << EntryAddress << "\n";
			if (OIRegion.size() == 0) continue;

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
		if (LLVMRegion->getEntryBlock().getFirstNonPHI()->getOpcode() != llvm::Instruction::Ret) {
			if (VerboseOutput)
				Module->print(llvm::errs(), nullptr);

			IRRegions[EntryAddress] = llvm::CloneModule(Module).release();

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

bool Manager::addOIRegion(uint32_t EntryAddress, OIInstList OIRegion, spp::sparse_hash_map<uint32_t, uint32_t> BrTargets) {
	if (!isRegionEntry(EntryAddress) && OIRegion.size() > 3 && OIRegions.count(EntryAddress) == 0) {
		OIRegionsMtx.lock();
		OIRegionsKey.push_back(EntryAddress);
		OIRegions[EntryAddress]   = OIRegion;
		OIBrTargets[EntryAddress] = BrTargets;
		OIRegionsMtx.unlock();
		return true;
	}
	return false;
}

int32_t Manager::jumpToRegion(uint32_t EntryAddress) {
	uint32_t JumpTo = EntryAddress;

	while (isNativeRegionEntry(JumpTo)) {
		uint32_t LastTo = JumpTo;

		uint32_t (*FP)(int32_t*, uint32_t*, volatile uint64_t*) = (uint32_t (*)(int32_t*, uint32_t*, volatile uint64_t*)) NativeRegions[JumpTo];
		JumpTo = FP(TheMachine.getRegisterPtr(), TheMachine.getMemoryPtr(), NativeRegions);
	}

	return JumpTo;
}
