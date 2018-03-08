#include <RFT.hpp>

#include <memory>

using namespace dbt;

void MethodBased::onBranch(Machine &M) {
  if (TheManager.isNativeRegionEntry(M.getPC())) {
    auto Next = TheManager.jumpToRegion(M.getPC(), M); 
    M.setPC(Next);
  } else if (M.isMethodEntry(M.getPC())) {
    startRegionFormation(M.getPC());
    for (uint32_t Addr = M.getPC(); Addr < M.getMethodEnd(M.getPC()); Addr += 4) 
      insertInstruction(Addr, M.getInstAt(Addr).asI_); 
    bool Inserted = finishRegionFormation();
    if (Inserted) {
      std::cout << "Compiling " << M.getMethodName(M.getPC()) << " " << M.getPC() << " -> " << M.getMethodEnd(M.getPC())<< " ("<< TheManager.getNumOfOIRegions() << ")\n";
      std::cout << "Waiting for it....";
      while (!TheManager.isNativeRegionEntry(M.getPC())) {}
      std::cout << "Done.\n";
      auto Next = TheManager.jumpToRegion(M.getPC(), M); 
      M.setPC(Next);
    }
  }
}
