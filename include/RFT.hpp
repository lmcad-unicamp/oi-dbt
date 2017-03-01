#ifndef RFT_h
#define RFT_h

#include <machine.hpp>

#include <unordered_map>
#include <vector>

#define OIInstList std::vector<uint32_t>

namespace dbt {
  class Machine;

  class RFT {
  protected:
    std::unordered_map<uint32_t, uint32_t> ExecFreq;
    std::unordered_map<uint32_t, OIInstList> OIRegions;
    bool Recording = false;
    uint32_t RecordingEntry;

  public:
    void printRegions(Machine&);

    virtual void onBranch(dbt::Machine&, uint32_t) = 0;
    virtual void onNextInst(dbt::Machine&) = 0;
  };

  class NET : public RFT {
  public:
    void onBranch(dbt::Machine&, uint32_t);
    void onNextInst(dbt::Machine&);
  };

  class NullRFT : public RFT {
  public:
    void onBranch(dbt::Machine&, uint32_t) {};
    void onNextInst(dbt::Machine&) {};
  };
}

#endif
