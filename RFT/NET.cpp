#include <RFT.hpp>

#include <memory>

using namespace dbt;

void NET::onBranch(Machine& M, uint32_t NewPC) {
  uint32_t PC = M.getPC();
  if (!Recording) { 
      ExecFreq[NewPC]++;
      if (ExecFreq[NewPC] > 50 && Regions.count(NewPC) == 0) {
        Recording = true; 
        RecordingEntry = NewPC;
        Regions[NewPC].push_back(NewPC);
      }
  } else {
    if (Regions.count(NewPC) != 0) {
      Recording = false;
    } else {
      Regions[RecordingEntry].push_back(NewPC);
    }
  }
}

void NET::onNextInst(Machine& M) {
  if (Recording) Regions[RecordingEntry].push_back(M.getPC()+4);
}

void NET::installRFT(Machine& M) {
  M.addNextInstEventListener(this);
  M.addBranchEventListener(this);
}
