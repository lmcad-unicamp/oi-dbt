#include <RFT.hpp>

#include <memory>

using namespace dbt;

void NETPlus::onBranch(Machine& M) {
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

void NETPlus::onNextInst(Machine& M) {
  if (TheManager.isRegionEntry(M.getPC())) {
    if (Recording) 
      finishRegionFormation(); 

    if (TheManager.isNativeRegionEntry(M.getPC())) {
      auto Next = TheManager.jumpToRegion(M.getPC(), M); 
      M.setPC(Next);
    }
  } else {
    if (Recording) {
      if (TheManager.isRegionEntry(M.getPC())) 
        finishRegionFormation(); 
      else 
        OIRegion.push_back({M.getPC(), M.getInstAtPC().asI_});
    }
  }
}
