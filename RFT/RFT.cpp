#include <RFT.hpp>
#include <OIPrinter.hpp>

#include <iostream>
#include <iomanip>

unsigned Total = 0;
void dbt::RFT::insertInstruction(uint32_t Addrs, uint32_t Opcode) {
  if (!hasRecordedAddrs(Addrs))
    OIRegion.push_back({Addrs, Opcode});
}

void dbt::RFT::insertInstruction(std::array<uint32_t, 2>& Inst) {
  insertInstruction(Inst[0], Inst[1]);
}

bool dbt::RFT::isBackwardLoop(uint32_t PC) {
  return OIRegion.size() == 0 ? false : PC < OIRegion.back()[0];  
}

void dbt::RFT::startRegionFormation(uint32_t PC) {
  while(TheManager.getNumOfOIRegions() != 0);
  if (TheManager.getNumOfOIRegions() == 0) {
    Recording = true;
    RecordingEntry = PC;
    OIRegion.clear();
    ExecFreq[PC] = 0;
  }
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

  if (OIRegion.size() != 0 && hasRecordedAddrs(RecordingEntry) && AlreadyCompiled.count(RecordingEntry) == 0) {
    Added = TheManager.addOIRegion(RecordingEntry, OIRegion);
    if (Added) {
      Total += OIRegion.size();
      AlreadyCompiled.insert(RecordingEntry);
    }
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
