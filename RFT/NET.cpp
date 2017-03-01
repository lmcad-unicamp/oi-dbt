#include <RFT.hpp>

#include <memory>

using namespace dbt;

void NET::onBranch(Machine& M, uint32_t NewPC) {
  uint32_t PC = M.getPC();
  if (!Recording) { 
      ExecFreq[NewPC]++;
      if (ExecFreq[NewPC] > 50 && OIRegions.count(NewPC) == 0) {
        Recording = true; 
        RecordingEntry = NewPC;
        OIRegions[NewPC].push_back(NewPC);
      }
  } else {
    if (OIRegions.count(NewPC) != 0)
      Recording = false;
    else
      OIRegions[RecordingEntry].push_back(NewPC);
  }
}

void NET::onNextInst(Machine& M) {
  if (Recording) OIRegions[RecordingEntry].push_back(M.getPC()+4);
}
