#ifndef RFT_h
#define RFT_h

#include <machine.hpp>
#include <manager.hpp>

#include <unordered_map>
#include <vector>
#include <array>

#define OIInstList std::vector<std::array<uint32_t,2>>

namespace dbt {
  class Manager;
  class Machine;

  class RFT {
  protected:
    std::unordered_map<uint32_t, uint8_t> ExecFreq;
    OIInstList OIRegion;

    bool Recording = false;
    uint32_t RecordingEntry;
    
    Manager& TheManager;
  public:
    RFT(Manager& M) : TheManager(M) {};

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

  class NullRFT : public RFT {
  public:
    NullRFT(Manager& M) : RFT(M) {};

    void onBranch(dbt::Machine&) {};
    void onNextInst(dbt::Machine&) {};
  };
}

#endif
