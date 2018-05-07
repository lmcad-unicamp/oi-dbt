#include <RFT.hpp>
#include <OIPrinter.hpp>

#include <memory>
#include <set>

using namespace dbt;

// Run perf record
// then perf report --stdio | sed '/^#/d' | awk '{print $1,$3,$5}' | sed '/^\s*$/d' | grep "a.out" | sed 's/%//g'

std::set<uint32_t> AlreadyCompiled;

unsigned NumOfInsts = 0;
void MethodBased::addFunctionToCompile(uint32_t PC, Machine &M) {
  if (AlreadyCompiled.count(PC) != 0) return;
  if (NumOfInsts > RegionLimitSize) return;

  if (M.isMethodEntry(PC)) {
    std::vector<uint32_t> PossibleEntries;
    PossibleEntries.push_back(PC);
    for (uint32_t Addr = PC; Addr < M.getMethodEnd(PC); Addr += 4) {
      auto I  = OIDecoder::decode(M.getInstAt(Addr).asI_);
      if (I.Type == OIDecoder::OIInstType::Call || I.Type == OIDecoder::OIInstType::Syscall || I.Type == OIDecoder::OIInstType::Callr || I.Type == OIDecoder::OIInstType::Ijmp) 
        if (AlreadyCompiled.count(Addr+4) == 0) 
          PossibleEntries.push_back(Addr+4);
    }

    for (auto PossibleEntry : PossibleEntries) {
        startRegionFormation(PossibleEntry);

        for (uint32_t Addr = PossibleEntry; Addr < M.getMethodEnd(PC); Addr += 4) {
          if (NumOfInsts == RegionLimitSize + 1) {
            NumOfInsts++;
          } else if (NumOfInsts == RegionLimitSize) {
            for (auto I : OIRegion) {
              std::cerr << std::hex << I[0] << ":\t" << dbt::OIPrinter::getString(OIDecoder::decode(I[1])) << "\n";
            }
            std::cerr <<"BLAH: " << PossibleEntry << "\n";
            NumOfInsts++;
            break;
          }

          if (NumOfInsts > RegionLimitSize) break;

          insertInstruction(Addr, M.getInstAt(Addr).asI_);
          NumOfInsts++;
        }

        bool Inserted = finishRegionFormation();
        AlreadyCompiled.insert(PossibleEntry);
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
  if (!TheManager.isNativeRegionEntry(M.getPC())) {
    addFunctionToCompile(M.getPC(), M);
    while (TheManager.getNumOfOIRegions() != 0) {}
  }

  if (!M.isPreheating() && TheManager.isNativeRegionEntry(M.getPC())) {
    auto Next = TheManager.jumpToRegion(M.getPC(), M);
    M.setPC(Next);
  }
}
