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
    std::set<uint32_t> AlreadyCompiled;
    unsigned HotnessThreshold = 128;
    uint8_t ExecFreq[1000000];
    bool isEntry[1000000];
    OIInstList OIRegion;

    bool Recording = false;
    uint32_t RecordingEntry;

    uint32_t LastTarget;

    Manager& TheManager;

    void startRegionFormation(uint32_t); 
    bool finishRegionFormation(); 
    bool isBackwardLoop(uint32_t); 
    void insertInstruction(uint32_t, uint32_t);
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
    };

    virtual void onBranch(dbt::Machine&) = 0;
  };

  class NET : public RFT {
    bool IsRelaxed;
  public:
    NET(Manager& M, bool Relaxed = false) : RFT(M), IsRelaxed(Relaxed) {};

    void onBranch(dbt::Machine&);
  };

  class MRET2 : public RFT {
    OIInstList RecordingBufferTmp1, RecordingBufferTmp2;

    uint32_t header;
    spp::sparse_hash_map<uint32_t, unsigned> phases;

    unsigned stored_index = 0;
    OIInstList stored[1000];

    bool IsRelaxed;
  public:
    MRET2(Manager& M, bool Relaxed = false) : RFT(M), IsRelaxed(Relaxed) {};

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

    bool IsExtendedRelaxed;
    bool IsCallExtended;

    std::vector<uint32_t> ShadowStack;
  public:
    NETPlus(Manager& M, bool ExtRelaxed = false, bool CallExtend = false) : 
      RFT(M), IsExtendedRelaxed(ExtRelaxed), IsCallExtended(CallExtend) {};

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
