#ifndef MANAGER_HPP
#define MANAGER_HPP

#include <IREmitter.hpp>
#include <IROpt.hpp>
#include <IRLazyJIT.hpp>
#include <machine.hpp>

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

      unsigned NumOfThreads;
      OptPolitic OptMode;

      std::unique_ptr<IREmitter> IRE;
      std::unique_ptr<IROpt> IRO;
      llvm::orc::IRLazyJIT* IRJIT;

    public:
      Manager(unsigned T, OptPolitic O) : NumOfThreads(T), OptMode(O) {
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();
        llvm::InitializeNativeTargetAsmParser();

        IRE = llvm::make_unique<IREmitter>();
        IRO = llvm::make_unique<IROpt>();
        IRJIT = new llvm::orc::IRLazyJIT();
      };

      void addOIRegion(uint32_t, OIInstList);

      int32_t jumpToRegion(uint32_t, dbt::Machine&);

      bool isRegionEntry(uint32_t EntryAddress) {
        return OIRegions.count(EntryAddress);
      }

      bool isNativeRegionEntry(uint32_t EntryAddress) {
        return NativeRegions.count(EntryAddress);
      }

      size_t getNumOfOIRegions() {
        return OIRegions.size();
      }

      float getAvgRegionsSize() {
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
