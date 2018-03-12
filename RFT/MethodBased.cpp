#include <RFT.hpp>

#include <memory>

using namespace dbt;

// Run perf record
// then perf report --stdio | sed '/^#/d' | awk '{print $1,$3,$5}' | sed '/^\s*$/d' | grep "a.out" | sed 's/%//g'

void MethodBased::addFunctionToCompile(uint32_t PC, Machine &M) {
  if (M.isMethodEntry(PC) && (!CompileOnlyHot || ToCompile.count(M.getMethodName(PC)) != 0)) {
    startRegionFormation(PC);
    for (uint32_t Addr = PC; Addr < M.getMethodEnd(PC); Addr += 4) 
      insertInstruction(Addr, M.getInstAt(Addr).asI_); 
    bool Inserted = finishRegionFormation();
    if (Inserted) {
      std::cout << "Compiling " << M.getMethodName(PC) << " " << PC << " -> " << M.getMethodEnd(PC)<< " ("<< TheManager.getNumOfOIRegions() << ")\n";
      std::cout << "Waiting for it....";
      while (!TheManager.isNativeRegionEntry(PC)) {}
      std::cout << "Done.\n";
    }
  }

  for (uint32_t Addr = PC + 4; Addr < M.getMethodEnd(PC); Addr += 4) {
    auto I  = OIDecoder::decode(M.getInstAt(Addr).asI_);
    if (I.Type == OIDecoder::OIInstType::Call) {
      if (M.isMethodEntry(I.Addrs) && !TheManager.isNativeRegionEntry(I.Addrs)) 
        addFunctionToCompile(I.Addrs, M); 
    }
  }
}

void MethodBased::onBranch(Machine &M) {
  if (!TheManager.isNativeRegionEntry(M.getPC())) 
    addFunctionToCompile(M.getPC(), M);
  auto Next = TheManager.jumpToRegion(M.getPC(), M); 
  M.setPC(Next);
}
