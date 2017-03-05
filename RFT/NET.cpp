#include <RFT.hpp>

#include <memory>

using namespace dbt;

void NET::onBranch(Machine& M, uint32_t NewPC) {
  if (NewPC < M.getPC()) {
    if (!Recording) { 
      ++ExecFreq[NewPC];
      if (ExecFreq[NewPC] > 50) {
        Recording = true; 
        RecordingEntry = NewPC;
      }
    } else {
      Recording = false;
      emitIR(RecordingEntry);
    }
  }
}

void NET::onNextInst(Machine& M) {
  if (Recording) {
    if (OIRegions.count(M.getPC()) != 0) { 
      Recording = false;
      emitIR(RecordingEntry);
    } else {
      OIRegions[RecordingEntry].push_back(M.getPC());
    }
  }
}
