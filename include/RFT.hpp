#ifndef RFT_h
#define RFT_h

#include <machine.hpp>
#include <manager.hpp>
#include <timer.hpp>

#include <unordered_map>
#include <vector>
#include <array>
#include <set>

#define OIInstList std::vector<std::array<uint32_t,2>>

namespace dbt {
  class Manager;
  class Machine;

  class RFT {
  protected:
    const unsigned HotnessThreshold = 20;
    std::unordered_map<uint32_t, uint8_t> ExecFreq;
    OIInstList OIRegion;
    std::set<uint32_t> OIAddrs;

    bool Recording = false;
    uint32_t RecordingEntry;
    
    Manager& TheManager;

    void startRegionFormation(uint32_t); 
    void finishRegionFormation(); 
    void insertInstruction(uint32_t, uint32_t);
    void insertInstruction(std::array<uint32_t, 2>&);
  public:
    RFT(Manager& M) : TheManager(M) {};

    ~RFT() {}

    void printRegions();

    virtual void onBranch(dbt::Machine&) = 0;
    virtual void onNextInst(dbt::Machine&) = 0;
  };

  class NET : public RFT {
  public:
    NET(Manager& M) : RFT(M) {};

    void onBranch(dbt::Machine&);
    void onNextInst(dbt::Machine&);
  };

  class MRET2 : public RFT {
  private:
    OIInstList RecordingBufferTmp1, RecordingBufferTmp2;

    uint32_t header;
    std::unordered_map<uint32_t, unsigned> phases;

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
    void onNextInst(dbt::Machine&);
  };

  class NETPlus : public RFT {
    void addNewPath(OIInstList);
    void expand(unsigned, Machine&);
    void expandAndFinish(Machine&);
  public:
    NETPlus(Manager& M) : RFT(M) {};

    void onBranch(dbt::Machine&);
    void onNextInst(dbt::Machine&);
  };

  class LEF : public RFT {
    std::unordered_map<uint32_t, bool> CamesFromCall;
    bool hasRet;

    void expand(uint32_t, Machine&);
    void endRecording(Machine&);
  public:
    LEF(Manager& M) : RFT(M), hasRet(false) {};

    void onBranch(dbt::Machine&);
    void onNextInst(dbt::Machine&);
  };

	class LEI : public RFT {
    #define MAX_SIZE_BUFFER 2000

		struct branch_t {
			uint32_t src;
			uint32_t tgt;
		};

		std::vector<branch_t> Buffer;
    std::unordered_map<uint32_t, int> BufferHash;

    void circularBufferInsert(uint32_t, uint32_t);
    void formTrace(uint32_t, int, Machine& M);
    bool isFollowedByExit(int);
  public:
    LEI(Manager& M) : RFT(M) {};

    void onBranch(dbt::Machine&);
    void onNextInst(dbt::Machine&);
  };

  class NullRFT : public RFT {
  public:
    NullRFT(Manager& M) : RFT(M) {};

    void onBranch(dbt::Machine&) {};
    void onNextInst(dbt::Machine&) {};
  };
}

#endif
