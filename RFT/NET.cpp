#include <RFT.hpp>

#include <memory>

using namespace dbt;

void NET::onBranch(Machine &M) {
  if (Recording) { 
    for (uint32_t I = LastTarget; I <= M.getLastPC(); I += 4) {
      if (TheManager.isRegionEntry(I)) {
        finishRegionFormation(); 
        break;
      }
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

    ++ExecFreq[Next];
    if (ExecFreq[M.getPC()] > HotnessThreshold)
      startRegionFormation(Next);
  } 

  LastTarget = M.getPC();
}
