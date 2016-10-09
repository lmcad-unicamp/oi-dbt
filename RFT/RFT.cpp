#include <RFT.hpp>

#include <iostream>
#include <iomanip>

void dbt::RFT::printRegions(Machine& M) {
  std::cout << std::endl << "\t\t NET" << std::endl;
  std::cout << std::endl << "Number of Regions: " << Regions.size() << std::endl;
  uint32_t AvgSize = 0;
  for (auto Region : Regions) {
    AvgSize += Region.second.size();
  }
  std::cout << "Average Region Static Size: " << (double)AvgSize/Regions.size() << std::endl;
  std::cout << "\tRegions:" << std::endl;

  int i = 1;
  for (auto Region : Regions) {
    std::cout << std::endl << "#" << i++ << std::endl;
    for (auto Addrs : Region.second) {
      dbt::Word W = M.getInstAt(Addrs);
      std::cout << std::hex << Addrs << "\t" << std::setw(8) << std::setfill('0')  
        << W.asI_ << "\t";
      W = {W.asC_[3], W.asC_[2], W.asC_[1], W.asC_[0]};

      uint8_t Op = W.asI_ >> 26;
      constexpr unsigned OpMask = 0x3FFFFFF;

      uint8_t Ext;
      switch(Op) {
      case 0b100010:
        Ext = (W.asI_ & OpMask) >> 12;
        if (Ext == 0b101) std::cout << "absd" << std::endl;
        break;
      case 0b100000:
        Ext = W.asI_ >> 18;
        if (Ext == 0b0) std::cout << "add" << std::endl;
        if (Ext == 0b1) std::cout << "ldihi" << std::endl;
        if (Ext == 0b110) std::cout << "and_" << std::endl;
        if (Ext == 0b111) std::cout << "or_" << std::endl;
        break;
      case 0b11111: 
        Ext = (W.asI_ & OpMask) >> 20;
        if (Ext == 0b100) std::cout << "ldi" << std::endl;
        break;
      case 0b110:
        std::cout << "ldw" << std::endl;
        break;
      case 0b1110:
        std::cout << "addi" << std::endl;
        break;
      case 0b1:
        std::cout << "call" << std::endl;
        break;
      case 0b100011:
        std::cout << "jumpr" << std::endl;
        break;
      case 0b1011:
        std::cout << "stw" << std::endl;
        break;
      case 0b10001:
        std::cout << "sltiu" << std::endl;
        break;
      case 0b10000:
        std::cout << "slti" << std::endl;
        break;
      case 0b10101:
        std::cout << "jeq" << std::endl;
        break;
      case 0b10110:
        std::cout << "jne" << std::endl;
        break;
      case 0b0:
        std::cout << "jump" << std::endl;
        break;
      case 0b100100:
        std::cout << "syscall" << std::endl;
        break;
      default:
        std::cout << "nop" << std::endl;
      }

    }
  }
}
