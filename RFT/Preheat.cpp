#include <RFT.hpp>

#include <memory>

using namespace dbt;

void PreheatRFT::onBranch(Machine &M) {
  if (TheManager.isNativeRegionEntry(M.getPC())) {
    auto Next = TheManager.jumpToRegion(M.getPC()); 
    M.setPC(Next);
  } 
}
