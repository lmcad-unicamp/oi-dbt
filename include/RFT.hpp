#ifndef RFT_h
#define RFT_h

#include <machine.hpp>
#include <manager.hpp>
#include <timer.hpp>

#include <sparsepp/spp.h>
#include <vector>
#include <array>
#include <set>
#include <fstream>

#define OIInstList std::vector<std::array<uint32_t,2>>

namespace dbt {
  class Manager;
  class Machine;

  class RFT {
  protected:
    unsigned HotnessThreshold = 50;
    spp::sparse_hash_map<uint32_t, uint8_t> ExecFreq;
    spp::sparse_hash_map<uint32_t, uint32_t> BranchesTargets;
    OIInstList OIRegion;

    bool Recording = false;
    uint32_t RecordingEntry;

    uint32_t LastTarget;

    unsigned RegionLimitSize = -1;
    
    Manager& TheManager;

    void startRegionFormation(uint32_t); 
    bool finishRegionFormation(); 
    void insertInstruction(uint32_t, uint32_t);
    void setBranchTarget(uint32_t, uint32_t);
    void insertInstruction(std::array<uint32_t, 2>&);
    bool hasRecordedAddrs(uint32_t);
  public:
    RFT(Manager& M) : TheManager(M) {};

    ~RFT() {}

    void printRegions();

    void setHotnessThreshold (unsigned int threshold) { 
      HotnessThreshold = threshold;
    };

    void setRegionLimitSize(unsigned Limit) {
      RegionLimitSize = Limit;
    };

    virtual void onBranch(dbt::Machine&) = 0;
  };

  class NET : public RFT {
  public:
    NET(Manager& M) : RFT(M) {};

    void onBranch(dbt::Machine&);
  };

  class MethodBased : public RFT {
    std::set<std::string> ToCompile;
    bool CompileOnlyHot = false;
   public:
    MethodBased(Manager& M, std::string PathToTCList = "") : RFT(M) {
      if (PathToTCList != "") {
        std::ifstream infile(PathToTCList);
        double FuncCoverage;
        std::string FuncName;
        double TotalCoverage = 0;
        while (infile >> FuncCoverage >> FuncName)  {
          ToCompile.insert(FuncName);
          CompileOnlyHot = true;
          TotalCoverage += FuncCoverage;
          if (TotalCoverage > 98) 
            break;
        } 
      }
    };

    void onBranch(dbt::Machine&);
  };

  class MRET2 : public RFT {
  private:
    OIInstList RecordingBufferTmp1, RecordingBufferTmp2;

    uint32_t header;
    spp::sparse_hash_map<uint32_t, unsigned> phases;

    unsigned stored_index = 0;
    OIInstList stored[1000];
  public:
    MRET2(Manager& M) : RFT(M) {};

    uint32_t getStoredIndex(uint32_t);
    uint32_t getPhase(uint32_t);
    bool hasRecorded(uint32_t);
    void mergePhases();
    void finishPhase();

    void onBranch(dbt::Machine&);
  };

  class NETPlus : public RFT {
    void addNewPath(OIInstList);
    void expand(unsigned, Machine&);
    void expandAndFinish(Machine&);
  public:
    NETPlus(Manager& M) : RFT(M) {};

    void onBranch(dbt::Machine&);
  };

	class LEI : public RFT {
    #define MAX_SIZE_BUFFER 2000

		struct branch_t {
			uint32_t src;
			uint32_t tgt;
		};

		std::vector<branch_t> Buffer;
    spp::sparse_hash_map<uint32_t, int> BufferHash;

    void circularBufferInsert(uint32_t, uint32_t);
    void formTrace(uint32_t, int, Machine& M);
    bool isFollowedByExit(int);
  public:
    LEI(Manager& M) : RFT(M) {};

    void onBranch(dbt::Machine&);
  };

  class NullRFT : public RFT {
  public:
    NullRFT(Manager& M) : RFT(M) {};

    void onBranch(dbt::Machine&) {};
  };

  class PreheatRFT : public RFT {
  public:
    PreheatRFT(Manager& M) : RFT(M) {};

    void onBranch(dbt::Machine&);
  };
}

#endif
