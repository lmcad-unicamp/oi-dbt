#include <RFT.hpp>
#include <OIPrinter.hpp>

#include <memory>

using namespace dbt;

unsigned TotalInst = 0;
void NET::onBranch(Machine &M) {
  if (Recording) { 
    if (OIDecoder::isIndirectBranch(OIDecoder::decode(M.getInstAt(M.getLastPC()).asI_)))
      setBranchTarget(M.getLastPC(), M.getPC());

    for (uint32_t I = LastTarget; I <= M.getLastPC(); I += 4) {
      if (TheManager.isRegionEntry(I) || 
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
      } else if (TotalInst == RegionLimitSize) {
        for (auto I : OIRegion) {
          std::cerr << std::hex << I[0] << ":\t" << dbt::OIPrinter::getString(OIDecoder::decode(I[1])) << "\n";
        }
        std::cerr <<"BLAH: " << I << "\n";
        TotalInst++;
        finishRegionFormation(); 
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
