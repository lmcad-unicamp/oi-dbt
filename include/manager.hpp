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
#include <OIPrinter.hpp>
#include <stack>

#include "llvm/Support/TargetSelect.h"
#include "llvm/Bitcode/BitcodeWriter.h"

#define OIInstList std::vector<std::array<uint32_t,2>>
#define NATIVE_REGION_SIZE 1000000

namespace dbt {
  class IREmitter;
  class Machine;
  class Manager {
    public:
      enum OptPolitic { None, Normal, Aggressive, Custom };

    private:
      llvm::LLVMContext TheContext;
      dbt::Machine& TheMachine;

      spp::sparse_hash_map<uint32_t, spp::sparse_hash_map<uint32_t, uint32_t>> OIBrTargets;
			std::vector<uint32_t> OIRegionsKey;
      spp::sparse_hash_map<uint32_t, OIInstList> OIRegions;
      std::unordered_map<uint32_t, OIInstList> CompiledOIRegions;
      std::vector<uint32_t> IRRegionsKey;
      spp::sparse_hash_map<uint32_t, llvm::Module*> IRRegions;
      std::vector<uint32_t> RegionAddresses;
      volatile uint64_t NativeRegions[NATIVE_REGION_SIZE];

      mutable std::shared_mutex OIRegionsMtx, IRRegionsMtx, NativeRegionsMtx, CompiledOIRegionsMtx;

      unsigned NumOfThreads;

      OptPolitic OptMode;
      std::unordered_map<uint32_t, std::vector<std::string>>* CustomOpts;

      uint32_t DataMemOffset;

      std::unique_ptr<IREmitter> IRE;
      std::unique_ptr<IROpt> IRO;
      llvm::orc::IRLazyJIT* IRJIT;

      std::atomic<bool> isRegionRecorging;
      std::atomic<bool> isRunning;
      std::atomic<bool> isFinished;
      std::vector<std::thread> ThreadPool;

      unsigned regionFrequency = 0;
      unsigned CompiledRegions = 0;
      unsigned OICompiled = 0;
      unsigned LLVMCompiled = 0;
      float AvgOptCodeSize = 0;

      bool VerboseOutput = false;

			std::unordered_map<uint32_t, llvm::Module*> ModulesLoaded;
      bool IsToLoadRegions = false;
      bool IsToLoadBCFormat = true;

      llvm::Module* loadRegionFromFile(std::string);
      void loadRegionsFromFiles();

      void runPipeline();

    public:
      Manager(unsigned T, uint32_t DMO, dbt::Machine& M, bool VO = false) : NumOfThreads(T), DataMemOffset(DMO),
         isRunning(true), isFinished(false), VerboseOutput(VO), TheMachine(M) {

        memset((void*) NativeRegions, 0, sizeof(NativeRegions));

        if (T) {
          ThreadPool.push_back(std::thread(&Manager::runPipeline, this));
        }
      }

      void dumpStats() {
        std::cerr << "Compiled Regions: " << std::dec << CompiledRegions << "\n";
        std::cerr << "Avg Code Size Reduction: ";
        std::cerr << CompiledRegions? AvgOptCodeSize/CompiledRegions : 0;
        std::cerr << std::endl;
        std::cerr << "Compiled OI: " << OICompiled << "\n";
        std::cerr << "Compiled LLVM: " << LLVMCompiled << std::endl;
      }

      ~Manager() {

        // Alert threads to stop
        isRunning = false;

        // Waits the thread finish
        if(NumOfThreads) {
          while (!isFinished) {}

          for (unsigned i = 0; i < NumOfThreads; i++) 
            if (ThreadPool[i].joinable())
              ThreadPool[i].join();
        }
      }

      void setOptPolicy(OptPolitic OM) {
        OptMode = OM;
      }

      void setCustomOpts(std::unordered_map<uint32_t, std::vector<std::string>>* COpts) {
        CustomOpts = COpts;
        OptMode = Custom;
      }
 
      unsigned getCompiledRegions (void){
        return CompiledRegions;
      }

      unsigned getOICompiled (void) {
        return OICompiled;
      }

      bool getRegionRecording (void) {
        return static_cast<bool>(isRegionRecorging);
      }

      void setRegionRecorging (bool value) {
        isRegionRecorging = value;
      }

      unsigned getLLVMCompiled (void) {
        return LLVMCompiled;
      }

      float getAvgOptCodeSize (void) {
        return AvgOptCodeSize;
      }

      void setToLoadRegions(bool AlreadLLVMFormat = true) {
        IsToLoadRegions = true;
        IsToLoadBCFormat = AlreadLLVMFormat; 
      }

     bool addOIRegion(uint32_t, OIInstList, spp::sparse_hash_map<uint32_t, uint32_t>);

      int32_t jumpToRegion(uint32_t);

      bool isRegionEntry(uint32_t EntryAddress) {
//        std::shared_lock<std::shared_mutex> lockOI(OIRegionsMtx);
//        std::shared_lock<std::shared_mutex> lockNative(NativeRegionsMtx);
        return OIRegions.count(EntryAddress) != 0 || NativeRegions[EntryAddress] != 0;
      }

      bool isNativeRegionEntry(uint32_t EntryAddress) {
        return (NativeRegions[EntryAddress] != 0);
      }

      size_t getNumOfOIRegions() {
        OIRegionsMtx.lock();
        size_t R = OIRegions.size();
        OIRegionsMtx.unlock();
        return R;
      }

      unsigned int getNumOfThreads(void){
        return NumOfThreads;
      }

      float getAvgRegionsSize() {
        uint64_t total;
        for (auto Region : OIRegions)
          total += Region.second.size();
        return (float)total / getNumOfOIRegions();
      }

      bool inCodeCache(uint32_t Addrs) {
        int i = 0;
        for (auto Region : OIRegions) {
          for (auto InstAddr : Region.second)
            if (InstAddr[0] == Addrs)
              return true;

            if(i > OIRegions.size())
              break;

            ++i;
        }
        return false;
      }

      OIInstList getCompiledOIRegion(uint32_t EntryAddrs) {
        return CompiledOIRegions[EntryAddrs];
      }

			void loadOIRegionsFromFiles();

      void mergeOIRegions();

      void dumpRegions(bool MergeRegions = false) {
        while (getNumOfOIRegions() != 0) {}
        std::cerr << "Dumping IR regions!\n";
        for (auto& M : IRRegions) {
          std::error_code EC;
          llvm::raw_fd_ostream OS("r"+std::to_string(M.first)+".bc", EC, llvm::sys::fs::F_None);
          WriteBitcodeToFile(M.second, OS);
          OS.flush(); 
        }
        if (MergeRegions) {
          std::cerr << "Merging OI regions!\n";
          mergeOIRegions();
        }
        std::cerr << "Dumping OI regions!\n";
        for (auto OIRegion : CompiledOIRegions) {
          std::error_code EC;
          llvm::raw_fd_ostream OS("r"+std::to_string(OIRegion.first)+".oi", EC, llvm::sys::fs::F_None);
          for (auto OIInsts : OIRegion.second) 
            OS << OIInsts[0] << "\t" << OIInsts[1] << "\t" << OIPrinter::getString(OIDecoder::decode(OIInsts[1])) <<  "\n"; 
          OS.flush();
        }
        std::error_code EC;
        llvm::raw_fd_ostream OS("regions.order", EC, llvm::sys::fs::F_None);
        for (auto A : IRRegionsKey) {
          OS << A << "\n";
        }
        OS.flush();
      }

      std::unordered_map<uint32_t, OIInstList>::iterator oiregions_begin() { return CompiledOIRegions.begin(); };
      std::unordered_map<uint32_t, OIInstList>::iterator oiregions_end()   { return CompiledOIRegions.end(); }; // FIXME: Not Thread Safe
  };
}

#endif
