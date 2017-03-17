#include <RFT.hpp>
#include <OIPrinter.hpp>

#include <iostream>
#include <iomanip>

void dbt::RFT::emitIR(uint32_t EntryAddr, uint32_t DataMemOffset) {
  IRRegions[EntryAddr] = LLVMEmitter.generateRegionIR(OIRegions[EntryAddr], DataMemOffset);
}

void dbt::RFT::printRegions() {
  std::cout << std::endl << "\t\t NET\n";
  std::cout << std::endl << "Number of Regions: " << OIRegions.size() << '\n';
  uint32_t AvgSize = 0;
  for (auto Region : OIRegions)
    AvgSize += Region.second.size();
  std::cout << "Average Region Static Size: " << (double)AvgSize/OIRegions.size() << std::endl;
  std::cout << "\tRegions:" << std::endl;

  int i = 1;
  for (auto Region : OIRegions) {
    std::cout << std::endl << "#" << i++ << '\n';
    for (auto Pair : Region.second) {
      auto Addrs = Pair[0];
      dbt::Word W;
      W.asI_ = Pair[1];
      std::cout << std::hex << Addrs << "\t" << std::setw(8) << std::setfill('0')  
        << W.asI_ << "\t";
      std::cout << OIPrinter::getString(OIDecoder::decode(W.asI_)) << "\n";
    }
  }
}
