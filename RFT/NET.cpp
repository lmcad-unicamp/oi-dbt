#include <RFT.hpp>

#include <memory>

using namespace dbt;

void NET::onBranch(Machine& M) {
  if (M.getPC() < M.getLastPC()) {
    if (!Recording) { 
      ++ExecFreq[M.getPC()];
      if (!TheManager.isRegionEntry(M.getPC()) && ExecFreq[M.getPC()] > 50) {
        Recording = true; 
        RecordingEntry = M.getPC();
      }
    } else {
      Recording = false;
      TheManager.addOIRegion(RecordingEntry, OIRegion);
      OIRegion.clear();
    }
  }
}

void NET::onNextInst(Machine& M) {
  if (TheManager.isNativeRegionEntry(M.getPC())) {
    if (Recording) {
      Recording = false;
      TheManager.addOIRegion(RecordingEntry, OIRegion);
      OIRegion.clear();
    }

    auto Next = TheManager.jumpToRegion(RecordingEntry, M);
    M.setPC(Next);
  } else {
    if (Recording) {
      if (TheManager.isRegionEntry(M.getPC())) { 
        Recording = false;
        TheManager.addOIRegion(RecordingEntry, OIRegion);
        OIRegion.clear();
      } else {
        OIRegion.push_back({M.getPC(), M.getInstAtPC().asI_});
      }
    }
  }
}
