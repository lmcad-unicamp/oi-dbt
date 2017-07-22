#include <RFT.hpp>

#include <memory>

using namespace dbt;

unsigned TotalInst = 0;
void NET::onBranch(Machine &M) {

  if (Recording) { 
    for (uint32_t I = LastTarget; I <= M.getLastPC(); I += 4) {
      if (TheManager.isRegionEntry(I) || M.getInstAt(I).asI_ == 0x90000001 || 
          OIDecoder::decode(M.getInstAt(I).asI_).Type == OIDecoder::OIInstType::Jumpr ||
          OIDecoder::decode(M.getInstAt(I).asI_).Type == OIDecoder::OIInstType::Sqrts ||
          OIDecoder::decode(M.getInstAt(I).asI_).Type == OIDecoder::OIInstType::Sqrtd) {
        finishRegionFormation(); 
        break;
      }
      if (TotalInst < RegionLimitSize) {
      if (hasRecordedAddrs(I)) {
        finishRegionFormation(); 
        break;
      }

      insertInstruction(I, M.getInstAt(I).asI_);
      TotalInst++;
      }
    }
    /*if (M.getPC() < M.getLastPC()) {
      finishRegionFormation(); 
    }*/
  } else if (M.getPC() < M.getLastPC()) {
    ++ExecFreq[M.getPC()];
    if (!TheManager.isRegionEntry(M.getPC()) && ExecFreq[M.getPC()] > HotnessThreshold) 
      startRegionFormation(M.getPC());
  }

  if (TheManager.isRegionEntry(M.getPC())) {
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
