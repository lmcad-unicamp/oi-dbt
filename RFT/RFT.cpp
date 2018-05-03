#include <RFT.hpp>
#include <OIPrinter.hpp>

#include <iostream>
#include <iomanip>

unsigned Total = 0;
void dbt::RFT::insertInstruction(uint32_t Addrs, uint32_t Opcode) {
  if (Total > RegionLimitSize) {
    finishRegionFormation();
    return;
  }

  if (!hasRecordedAddrs(Addrs))
    OIRegion.push_back({Addrs, Opcode});
}

void dbt::RFT::setBranchTarget(uint32_t BranchAddrs, uint32_t Target) {
  BranchesTargets[BranchAddrs] = Target;
}

void dbt::RFT::insertInstruction(std::array<uint32_t, 2>& Inst) {
  insertInstruction(Inst[0], Inst[1]);
}

void dbt::RFT::startRegionFormation(uint32_t PC) {
  Recording = true;
  RecordingEntry = PC;
  OIRegion.clear();
  BranchesTargets.clear();
  ExecFreq[PC] = 0;
}

bool dbt::RFT::hasRecordedAddrs(uint32_t Addrs) {
  for (auto I : OIRegion){
    if (I[0] == Addrs) 
      return true;
  }
  return false;
}

bool dbt::RFT::finishRegionFormation() {
  bool Added = false;
  if (OIRegion.size() > 0 && hasRecordedAddrs(RecordingEntry)) {
    Added = TheManager.addOIRegion(RecordingEntry, OIRegion, BranchesTargets);
    if (Added)
      Total += OIRegion.size();
  }
  OIRegion.clear();
  Recording = false;
  return Added;
}

void dbt::RFT::printRegions() {
  std::cout << std::endl << "\t\t NET\n";

  int i = 1;
  for (auto Region = TheManager.oiregions_begin(); Region != TheManager.oiregions_end(); Region++) {
    std::cout << std::endl << "#" << i++ << " entry: " << std::hex << Region->first << '\n';
    for (auto Pair : Region->second) {
      auto Addrs = Pair[0];
      dbt::Word W;
      W.asI_ = Pair[1];
      std::cout << std::hex << Addrs << "\t" << std::setw(8) << std::setfill('0')
        << W.asI_ << "\t";
      std::cout << OIPrinter::getString(OIDecoder::decode(W.asI_)) << "\n";
    }
  }
  std::cout << "\n";
}
