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
#include <sparsepp/spp.h>

#include "llvm/Support/TargetSelect.h"

#define OIInstList std::vector<std::array<uint32_t,2>>

namespace dbt {
  class Machine;
  class Manager {
    public:
      enum OptPolitic { None, Normal, Aggressive };

    private:
      llvm::LLVMContext TheContext;

      spp::sparse_hash_map<uint32_t, OIInstList> OIRegions;
      std::unordered_map<uint32_t, OIInstList> CompiledOIRegions;
      spp::sparse_hash_map<uint32_t, llvm::Module*> IRRegions;
      intptr_t NativeRegions[100000];
      //spp::sparse_hash_map<uint32_t, intptr_t> NativeRegions;

      mutable std::shared_mutex OIRegionsMtx, IRRegionsMtx, NativeRegionsMtx, CompiledOIRegionsMtx;

      unsigned NumOfThreads;
      OptPolitic OptMode;

      uint32_t DataMemOffset;

      std::unique_ptr<IREmitter> IRE;
      std::unique_ptr<IROpt> IRO;
      llvm::orc::IRLazyJIT* IRJIT;

      std::atomic<bool> isRunning;
      std::thread Thr;

      unsigned CompiledRegions = 0;
      unsigned OICompiled = 0;
      unsigned LLVMCompiled = 0;
      float AvgOptCodeSize = 0;
      
      void runPipeline();

    public:
      Manager(unsigned T, OptPolitic O, uint32_t DMO) : NumOfThreads(T), OptMode(O), DataMemOffset(DMO), isRunning(true), 
                                          Thr(&Manager::runPipeline, this) {}

      ~Manager() {
        isRunning = false;
        if (Thr.joinable())
          Thr.join();

        std::cout << "Compiled Regions: " << std::dec << CompiledRegions << "\n";
        std::cout << "Avg Code Size Reduction: " << AvgOptCodeSize/CompiledRegions << "\n";
        std::cout << "Compiled OI: " << OICompiled << "\n";
        std::cout << "Compiled LLVM: " << LLVMCompiled << "\n";
      }

      void addOIRegion(uint32_t, OIInstList);

      int32_t jumpToRegion(uint32_t, dbt::Machine&);

      bool isRegionEntry(uint32_t EntryAddress) {
        std::shared_lock<std::shared_mutex> lockOI(OIRegionsMtx);
        std::shared_lock<std::shared_mutex> lockNative(NativeRegionsMtx);
        return OIRegions.count(EntryAddress) != 0 || NativeRegions[EntryAddress] != 0;//NativeRegions.count(EntryAddress) != 0;
      }

      bool isNativeRegionEntry(uint32_t EntryAddress) {
        //std::shared_lock<std::shared_mutex> lock(NativeRegionsMtx);
        return NativeRegions[EntryAddress] != 0; //NativeRegions.count(EntryAddress) != 0;
      }

      size_t getNumOfOIRegions() {
        //std::shared_lock<std::shared_mutex> lock(OIRegionsMtx);
        return OIRegions.size();
      }

      float getAvgRegionsSize() {
        //std::shared_lock<std::shared_mutex> lock(OIRegionsMtx);
        uint64_t total;
        for (auto Region : OIRegions) 
          total += Region.second.size();
        return (float)total / getNumOfOIRegions(); 
      }

      std::vector<uint32_t> getDirectTransitions(uint32_t EntryAddrs) {
        return IRE->getDirectTransitions(EntryAddrs);
      }

      OIInstList getCompiledOIRegion(uint32_t EntryAddrs) {
        //std::shared_lock<std::shared_mutex> lock(CompiledOIRegionsMtx);
        return CompiledOIRegions[EntryAddrs];
      }

      std::unordered_map<uint32_t, OIInstList>::iterator oiregions_begin() { return CompiledOIRegions.begin(); };
      std::unordered_map<uint32_t, OIInstList>::iterator oiregions_end()   { return CompiledOIRegions.end(); }; // FIXME: Not Thread Safe
  };
}

#endif
