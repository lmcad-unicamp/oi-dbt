#include <RFT.hpp>

#include <iostream>
#include <iomanip>

void dbt::RFT::printRegions(Machine& M) {
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
    for (auto Addrs : Region.second) {
      dbt::Word W = M.getInstAt(Addrs);
      std::cout << std::hex << Addrs << "\t" << std::setw(8) << std::setfill('0')  
        << W.asI_ << "\t";
      W = {W.asC_[0], W.asC_[1], W.asC_[2], W.asC_[3]};

      uint8_t Op = W.asI_ >> 26;
      constexpr unsigned OpMask = 0x3FFFFFF;

      uint8_t Ext;
      switch(Op) {
      case 0b100010:
        Ext = (W.asI_ & OpMask) >> 12;
        if (Ext == 0b101) std::cout << "absd";
        break;
      case 0b100000:
        Ext = W.asI_ >> 18;
        if (Ext == 0b0) std::cout << "add" ;
        if (Ext == 0b1) std::cout << "ldihi" ;
        if (Ext == 0b110) std::cout << "and_" ;
        if (Ext == 0b111) std::cout << "or_" ;
        break;
      case 0b11111: 
        Ext = (W.asI_ & OpMask) >> 20;
        if (Ext == 0b100) std::cout << "ldi" ;
        break;
      case 0b110:
        std::cout << "ldw" ;
        break;
      case 0b1110:
        std::cout << "addi" ;
        break;
      case 0b1:
        std::cout << "call" ;
        break;
      case 0b100011:
        std::cout << "jumpr" ;
        break;
      case 0b1011:
        std::cout << "stw" ;
        break;
      case 0b10001:
        std::cout << "sltiu" ;
        break;
      case 0b10000:
        std::cout << "slti" ;
        break;
      case 0b10101:
        std::cout << "jeq" ;
        break;
      case 0b10110:
        std::cout << "jne" ;
        break;
      case 0b0:
        std::cout << "jump" ;
        break;
      case 0b100100:
        std::cout << "syscall" ;
        break;
      default:
        std::cout << "nop" ;
      }
      std::cout << '\n';
    }
  }
}
