#include <RFT.hpp>
#include <OIPrinter.hpp>

#include <iostream>
#include <iomanip>

void dbt::RFT::insertInstruction(uint32_t Addrs, uint32_t Opcode) {
  if (OIAddrs.count(Addrs) == 0) {
    OIAddrs.insert(Addrs);
    OIRegion.push_back({Addrs, Opcode});
  }
}

void dbt::RFT::insertInstruction(std::array<uint32_t, 2>& Inst) {
  insertInstruction(Inst[0], Inst[1]);
}

void dbt::RFT::startRegionFormation(uint32_t PC) {
  Recording = true; 
  RecordingEntry = PC;
}

void dbt::RFT::finishRegionFormation() {
  if (OIRegion.size() > 0) {
    TheManager.addOIRegion(RecordingEntry, OIRegion);
    OIRegion.clear();
  }
  Recording = false;
}

void dbt::RFT::printRegions() {
  std::cout << std::endl << "\t\t NET\n";

  int i = 1;
  for (auto Region = TheManager.oiregions_begin(); Region != TheManager.oiregions_end(); Region++) {
    std::cout << std::endl << "#" << i++ << '\n';
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
