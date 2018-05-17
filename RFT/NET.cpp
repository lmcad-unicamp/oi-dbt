#include <RFT.hpp>
#include <OIPrinter.hpp>

#include <memory>

using namespace dbt;

//#define LIMITED

#ifdef LIMITED
unsigned TotalInst1 = 0;
#endif

void NET::onBranch(Machine &M) {
  if (Recording) { 
#ifdef LIMITED
    if (TotalInst1 <= RegionLimitSize) {
#endif

      for (uint32_t I = LastTarget; I <= M.getLastPC(); I += 4) {
        if (TheManager.isRegionEntry(I) || (IsRelaxed && hasRecordedAddrs(I))
            || (!IsRelaxed && (M.getPC() < M.getLastPC()))) { 

          finishRegionFormation(); 
          break;
        }

#ifdef LIMITED      
        if (TotalInst1 < RegionLimitSize) {
          insertInstruction(I, M.getInstAt(I).asI_);
          TotalInst1++;
        } else if (TotalInst1 == RegionLimitSize) {
          for (auto I : OIRegion) {
            std::cerr << std::hex << I[0] << ":\t" << dbt::OIPrinter::getString(OIDecoder::decode(I[1])) << "\n";
          }
          std::cerr <<"BLAH: " << I << "\n";
          TotalInst1++;
          finishRegionFormation(); 
          while (TheManager.getNumOfOIRegions() != 0) {}//isNativeRegionEntry(PossibleEntry)) {}
        }
#else
        insertInstruction(I, M.getInstAt(I).asI_);
#endif      
      }
#ifdef LIMITED
    }
#endif
  } else if (M.getPC() - M.getLastPC()) {
    ++ExecFreq[M.getPC()];
    if (!TheManager.isRegionEntry(M.getPC()) && ExecFreq[M.getPC()] > HotnessThreshold) 
      startRegionFormation(M.getPC());
  }

  if (TheManager.isNativeRegionEntry(M.getPC())) {
    if (Recording) 
      finishRegionFormation(); 

    auto Next = TheManager.jumpToRegion(M.getPC()); 
    M.setPC(Next);

    ++ExecFreq[Next];
    if (ExecFreq[M.getPC()] > HotnessThreshold) 
      startRegionFormation(Next);
  } 

  LastTarget = M.getPC();
}
