#include <elfio/elfio.hpp>
#include <machine.hpp>

#include <cstring>
#include <iostream>
#include <cassert>
#include <iomanip>

using namespace dbt;

#define NDEBUG

void
copystr(std::unique_ptr<char[]>& Target, const char* Source, uint32_t Size) {
  for (uint32_t i = 0; i < Size; ++i)
    Target[i] = Source[i];
}

void Machine::
setCodeMemory(uint32_t StartAddress, uint32_t Size, const char* CodeBuffer) {
  CodeMemOffset = StartAddress;
  CodeMemory = uptr<Word[]>(new Word[Size]);
  CodeMemLimit = Size + CodeMemOffset;
  for (uint32_t i = 0; i < Size; i++) {
    Word Bytes = {CodeBuffer[i], CodeBuffer[i+1],
                  CodeBuffer[i+2], CodeBuffer[i+3]};
    CodeMemory[i] = Bytes;
  }
}

void Machine::
setDataMemory(uint32_t StartAddress, uint32_t Size, const char* DataBuffer) {
  DataMemOffset = StartAddress;
  DataMemory = std::unique_ptr<char[]>(new char[Size]);
  DataMemLimit = Size + DataMemOffset;
  copystr(DataMemory, DataBuffer, Size);
}

uint32_t Machine::getPC() {
  return PC;
}

void Machine::incPC() {
  //if (Recording) Regions[RecordingEntry].push_back(PC+4);
  PC += 4;
}

void Machine::setPC(uint32_t NewPC) {
  assert((NewPC >= CodeMemOffset && NewPC < CodeMemLimit) &&
      "Jumping for an address out of border!");

/*  if (!Recording) { 
    if ((NewPC - PC) < -4) {
      ExecFreq[NewPC]++;
      if (ExecFreq[NewPC] > 50 && Regions.count(NewPC) == 0) {
        Recording = true; 
        RecordingEntry = NewPC;
        Regions[NewPC].push_back(NewPC);
      }
    }
  } else {
    if ((NewPC - PC) < -4 || Regions.count(NewPC) != 0) {
      Recording = false;
    } else {
      Regions[RecordingEntry].push_back(NewPC);
    }
  }*/

  PC = NewPC;
}

Word Machine::getInstAt(uint32_t Addr) {
  assert((Addr >= CodeMemOffset && Addr < CodeMemLimit) &&
      "Trying to access an address out of border!");

  uint32_t CorrectAddr = Addr - CodeMemOffset;

  return CodeMemory[CorrectAddr];
}

Word Machine::getInstAtPC() {
  return getInstAt(PC);
}

Word Machine::getNextInst() {
  ++PC;
  return getInstAtPC();
}

Word Machine::getMemValueAt(uint32_t Addr) {
  assert((Addr >= DataMemOffset && Addr < DataMemLimit) &&
      "Trying to access an address out of border!");

  uint32_t CorrectAddr = Addr - DataMemOffset;
  Word Bytes = {DataMemory[CorrectAddr+3], DataMemory[CorrectAddr+2],
    DataMemory[CorrectAddr+1], DataMemory[CorrectAddr]};

  return Bytes;
}

void Machine::setMemValueAt(uint32_t Addr, uint32_t Value) {
  assert((Addr >= DataMemOffset && Addr < DataMemLimit) &&
      "Trying to access an address out of border!");

  uint32_t CorrectAddr = Addr - DataMemOffset;
  DataMemory[CorrectAddr] = (char) Value >> 24;
  DataMemory[CorrectAddr+1] = (char) Value >> 16;
  DataMemory[CorrectAddr+2] = (char) Value >> 8;
  DataMemory[CorrectAddr+3] = (char) Value;
}

uint32_t Machine::getNumInst() {
  return (CodeMemLimit - CodeMemOffset)/4;
}

uint32_t Machine::getCodeStartAddrs() {
  return CodeMemOffset;
}

uint32_t Machine::getCodeEndAddrs() {
  return CodeMemLimit;
}

uint32_t Machine::getRegister(uint8_t R) {
  return Register[R];
}

void Machine::setRegister(uint8_t R, uint32_t V) {
  Register[R] = V;
}

uint32_t Machine::getDoubleRegister(uint8_t R) {
  return DoubleRegister[R];
}

void Machine::setDoubleRegister(uint8_t R, double V) {
  DoubleRegister[R] = V;
}

using namespace ELFIO;

#define STACK_SIZE 2024
int Machine::loadELF(const std::string ElfPath) {
  elfio reader;

  if (!reader.load(ElfPath))
    return 0;

  Elf_Half sec_num = reader.sections.size();
  for (int i = 0; i < sec_num; ++i) {
    section* psec = reader.sections[i];

    if (psec->get_name() == ".text") 
      setCodeMemory(psec->get_address(), psec->get_size(),  psec->get_data());
    else if (psec->get_name() == ".data") 
      setDataMemory(psec->get_address(), psec->get_size()+STACK_SIZE, psec->get_data());
  }

  if (!DataMemory) {
    DataMemLimit = STACK_SIZE;
    DataMemOffset = 0;
    DataMemory = uptr<char[]>(new char[STACK_SIZE]);
  }

  setRegister(29, (DataMemLimit-DataMemOffset-STACK_SIZE)+(STACK_SIZE/2)); //StackPointer
  setRegister(30, (DataMemLimit-DataMemOffset-STACK_SIZE)+(STACK_SIZE/2)); //StackPointer
  setRegister(0, 0);
  setPC(reader.get_entry());

  return 1;
}

void Machine::printRegions() {
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
      dbt::Word W = getInstAt(Addrs);
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
