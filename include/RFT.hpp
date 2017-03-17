#ifndef RFT_h
#define RFT_h

#include <machine.hpp>
#include <IREmitter.hpp>

#include <unordered_map>
#include <vector>
#include <array>

#define OIInstList std::vector<std::array<uint32_t,2>>

namespace dbt {
  class Machine;

  class RFT {
  protected:
    std::unordered_map<uint32_t, uint32_t> ExecFreq;
    std::unordered_map<uint32_t, OIInstList> OIRegions;
    std::unordered_map<uint32_t, llvm::Function*> IRRegions;

    bool Recording = false;
    uint32_t RecordingEntry;
    
    IREmitter LLVMEmitter;
  public:
    void emitIR(uint32_t, uint32_t); 

    void printRegions();

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
