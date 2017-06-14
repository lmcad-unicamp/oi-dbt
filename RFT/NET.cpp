#include <RFT.hpp>

#include <memory>

using namespace dbt;

unsigned Total = 0;
void NET::onBranch(Machine& M) {
  if (Recording) {

    for (uint32_t I = LastTarget; I <= M.getLastPC(); I += 4) {
      if (Total > RegionLimitSize) {
        finishRegionFormation();
        return;
      }
      Total++;
      insertInstruction(I, M.getInstAt(I).asI_);
    }
  }

  if (M.getPC() < M.getLastPC()) {
    if (!Recording) { 
      ++ExecFreq[M.getPC()];
      if (!TheManager.isRegionEntry(M.getPC()) && ExecFreq[M.getPC()] > HotnessThreshold) 
        startRegionFormation(M.getPC());
    } else {
      finishRegionFormation(); 
    }
  } 
  
  if (TheManager.isNativeRegionEntry(M.getPC())) {
    if (Recording) 
      finishRegionFormation(); 

    auto Next = TheManager.jumpToRegion(M.getPC(), M); 
    M.setPC(Next);

    ++ExecFreq[M.getPC()];
    if (ExecFreq[M.getPC()] > HotnessThreshold) {
      startRegionFormation(M.getPC());
    }
  } 

  LastTarget = M.getPC();
}
