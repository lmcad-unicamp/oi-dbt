#include <RFT.hpp>

#include <memory>

using namespace dbt;

void MethodBased::onBranch(Machine &M) {
  if (TheManager.isNativeRegionEntry(M.getPC())) {
    auto Next = TheManager.jumpToRegion(M.getPC(), M); 
    M.setPC(Next);
  } else if (M.isMethodEntry(M.getPC())) {
    std::cout << "Compiling " << M.getMethodName(M.getPC()) << "\n";
    startRegionFormation(M.getPC());
    for (uint32_t Addr = M.getPC(); Addr < M.getMethodEnd(M.getPC()); Addr += 4) 
      insertInstruction(Addr, M.getInstAt(Addr).asI_); 
    finishRegionFormation();

    while (!TheManager.isNativeRegionEntry(M.getPC())) {
      auto Next = TheManager.jumpToRegion(M.getPC(), M); 
      M.setPC(Next);
    }
  }
}
