#include <RFT.hpp>

#include <memory>

using namespace dbt;

// Run perf record
// then perf report --stdio | sed '/^#/d' | awk '{print $1,$3,$5}' | sed '/^\s*$/d' | grep "a.out" | sed 's/%//g'

unsigned NumOfInsts = 0;
void MethodBased::addFunctionToCompile(uint32_t PC, Machine &M) {
  if (NumOfInsts > RegionLimitSize) return;
  if (M.isMethodEntry(PC)) {
    std::vector<uint32_t> PossibleEntries;
    PossibleEntries.push_back(PC);
    for (uint32_t Addr = PC; Addr < M.getMethodEnd(PC); Addr += 4) {
      auto I  = OIDecoder::decode(M.getInstAt(Addr).asI_);
      if (I.Type == OIDecoder::OIInstType::Call || I.Type == OIDecoder::OIInstType::Syscall || I.Type == OIDecoder::OIInstType::Callr || I.Type == OIDecoder::OIInstType::Ijmp || I.Type == OIDecoder::OIInstType::Bc1f || I.Type == OIDecoder::OIInstType::Bc1t)
        PossibleEntries.push_back(Addr+4);
    }

    for (auto PossibleEntry : PossibleEntries) {
      startRegionFormation(PossibleEntry);

      for (uint32_t Addr = PossibleEntry; Addr < M.getMethodEnd(PC); Addr += 4) {
        if (NumOfInsts == RegionLimitSize + 1) {
          NumOfInsts++;
        }

        if (NumOfInsts > RegionLimitSize) break;
        insertInstruction(Addr, M.getInstAt(Addr).asI_);
        NumOfInsts++;
      }

      bool Inserted = finishRegionFormation();

      if (Inserted)
        while (!TheManager.isNativeRegionEntry(PossibleEntry)) {}
    }

    for (uint32_t Addr = PC + 4; Addr < M.getMethodEnd(PC); Addr += 4) {
      auto I  = OIDecoder::decode(M.getInstAt(Addr).asI_);
      if (I.Type == OIDecoder::OIInstType::Call) {
        uint32_t Target = ((Addr & 0xF0000000) | I.Addrs << 2);
        if (M.isMethodEntry(Target) && !TheManager.isNativeRegionEntry(Target))
          addFunctionToCompile(Target, M);
      }
    }
  }
}

void MethodBased::onBranch(Machine &M) {
  if (!TheManager.isNativeRegionEntry(M.getPC()))
    addFunctionToCompile(M.getPC(), M);

  if (!M.isPreheating() && TheManager.isNativeRegionEntry(M.getPC())) {
    auto Next = TheManager.jumpToRegion(M.getPC(), M);
    M.setPC(Next);
  }
}
