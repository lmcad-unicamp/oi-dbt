#include <RFT.hpp>

#include <memory>

using namespace dbt;

void NET::onBranch(Machine& M) {
  if (M.getPC() < M.getLastPC()) {
    if (!Recording) { 
      ++ExecFreq[M.getPC()];
      if (!TheManager.isRegionEntry(M.getPC()) && ExecFreq[M.getPC()] > 50) 
        startRegionFormation(M.getPC());
    } else {
      finishRegionFormation(); 
    }
  }
}

void NET::onNextInst(Machine& M) {
  if (TheManager.isNativeRegionEntry(M.getPC())) {
    if (Recording) 
      finishRegionFormation(); 

    auto Next = TheManager.jumpToRegion(RecordingEntry, M); // FIXME: This should not be here!
    M.setPC(Next);
  } else {
    if (Recording) {
      if (TheManager.isRegionEntry(M.getPC())) 
        finishRegionFormation(); 
      else 
        OIRegion.push_back({M.getPC(), M.getInstAtPC().asI_});
    }
  }
}
