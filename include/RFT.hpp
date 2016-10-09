#ifndef RFTS_h
#define RFTS

#include <machine.hpp>

#include <unordered_map>
#include <vector>

namespace dbt {
  class RFT : public NextInstEventListener, public BranchEventListener {
  protected:
    std::unordered_map<uint32_t, uint32_t> ExecFreq;
    std::unordered_map<uint32_t, std::vector<uint32_t>> Regions;
    bool Recording = false;
    uint32_t RecordingEntry;

  public:
    virtual void installRFT(dbt::Machine&) = 0;

    void printRegions(Machine&);
  };

  class NET : public RFT {
  public:
    void onBranch(dbt::Machine&, uint32_t);
    void onNextInst(dbt::Machine&);

    void installRFT(dbt::Machine&);
  };

  class MRET2 : public RFT {
  public:
    void onBranch(dbt::Machine&, uint32_t);
    void onNextInst(dbt::Machine&);

    void installRFT(dbt::Machine&);
  };

  class LEF : public RFT {
  public:
    void onBranch(dbt::Machine&, uint32_t);
    void onNextInst(dbt::Machine&);

    void installRFT(dbt::Machine&);
  };
}

#endif
