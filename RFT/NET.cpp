#include <RFT.hpp>

#include <memory>

using namespace dbt;

void NET::onBranch(Machine& M) {
  if (M.getPC() < M.getLastPC()) {
    if (!Recording) { 
      ++ExecFreq[M.getPC()];
      if (!TheManager.isRegionEntry(M.getPC()) && ExecFreq[M.getPC()] > HotnessThreshold) 
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

    auto Next = TheManager.jumpToRegion(M.getPC(), M); 
    M.setPC(Next);
  } else {
    if (Recording) 
      insertInstruction(M.getPC(), M.getInstAtPC().asI_);
  }
}
