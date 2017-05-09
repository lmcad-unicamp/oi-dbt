#ifndef MANAGER_HPP
#define MANAGER_HPP

#include <IREmitter.hpp>
#include <IROpt.hpp>
#include <IRLazyJIT.hpp>
#include <machine.hpp>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <timer.hpp>

#include "llvm/Support/TargetSelect.h"

#define OIInstList std::vector<std::array<uint32_t,2>>

namespace dbt {
  class Machine;
  class Manager {
    public:
      enum OptPolitic { None, Normal, Aggressive };

    private:
      llvm::LLVMContext TheContext;

      std::unordered_map<uint32_t, OIInstList> OIRegions;
      std::unordered_map<uint32_t, llvm::Module*> IRRegions;
      std::unordered_map<uint32_t, intptr_t> NativeRegions;

      mutable std::shared_mutex OIRegionsMtx, IRRegionsMtx, NativeRegionsMtx;

      uint32_t DataMemOffset;

      unsigned NumOfThreads;
      OptPolitic OptMode;

      std::unique_ptr<IREmitter> IRE;
      std::unique_ptr<IROpt> IRO;
      llvm::orc::IRLazyJIT* IRJIT;

      std::thread Thr;
      std::atomic<bool> isRunning;

      unsigned CompiledRegions = 0;
      unsigned OICompiled = 0;
      unsigned LLVMCompiled = 0;
      float AvgOptCodeSize = 0;
      
      dbt::Timer CompilerTimer;
    public:
      void runPipeline();

      Manager(unsigned T, OptPolitic O, uint32_t DMO) : NumOfThreads(T), OptMode(O), isRunning(true), 
                                          Thr(&Manager::runPipeline, this), DataMemOffset(DMO) {}

      ~Manager() {
        isRunning = false;
        if (Thr.joinable())
          Thr.join();

        CompilerTimer.printReport("Compilation");
        std::cout << "Compiled Regions: " << CompiledRegions << "\n";
        std::cout << "Avg Code Size Reduction: " << AvgOptCodeSize/CompiledRegions << "\n";
        std::cout << "Compiled OI: " << OICompiled << "\n";
        std::cout << "Compiled LLVM: " << LLVMCompiled << "\n";
      }

      void addOIRegion(uint32_t, OIInstList);

      int32_t jumpToRegion(uint32_t, dbt::Machine&);

      bool isRegionEntry(uint32_t EntryAddress) {
        std::shared_lock<std::shared_mutex> lockOI(OIRegionsMtx);
        std::shared_lock<std::shared_mutex> lockNative(NativeRegionsMtx);
        return OIRegions.count(EntryAddress) != 0 || NativeRegions.count(EntryAddress) != 0;
      }

      bool isNativeRegionEntry(uint32_t EntryAddress) {
        std::shared_lock<std::shared_mutex> lock(NativeRegionsMtx);
        return NativeRegions.count(EntryAddress) != 0;
      }

      size_t getNumOfOIRegions() {
        std::shared_lock<std::shared_mutex> lock(OIRegionsMtx);
        return OIRegions.size();
      }

      float getAvgRegionsSize() {
        std::shared_lock<std::shared_mutex> lock(OIRegionsMtx);
        uint64_t total;
        for (auto Region : OIRegions) 
          total += Region.second.size();
        return (float)total / getNumOfOIRegions(); 
      }

      std::unordered_map<uint32_t, OIInstList>::iterator oiregions_begin() { return OIRegions.begin(); };
      std::unordered_map<uint32_t, OIInstList>::iterator oiregions_end()   { return OIRegions.end(); };
  };
}

#endif
