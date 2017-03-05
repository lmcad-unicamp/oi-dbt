#include <elfio/elfio.hpp>
#include <machine.hpp>

#include <cstring>
#include <cassert>

using namespace dbt;

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
  PC += 4;
  ImplRFT.onNextInst(*this);
}

void Machine::setPC(uint32_t NewPC) {
  assert((NewPC >= CodeMemOffset && NewPC < CodeMemLimit) &&
      "Jumping for an address out of border!");

  ImplRFT.onBranch(*this, NewPC);
  PC = NewPC;
  ImplRFT.onNextInst(*this);
}

Word Machine::getInstAt(uint32_t Addr) {
  assert((Addr >= CodeMemOffset && Addr < CodeMemLimit) &&
      "Trying to access an address out of border!");

  return CodeMemory[Addr - CodeMemOffset];
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
  DataMemory[CorrectAddr]   = (Value >> 24) & 0xFF;
  DataMemory[CorrectAddr+1] = (Value >> 16) & 0xFF;
  DataMemory[CorrectAddr+2] = (Value >> 8) & 0xFF;
  DataMemory[CorrectAddr+3] = (Value) & 0xFF;
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

int32_t Machine::getRegister(uint8_t R) {
  return Register[R];
}

void Machine::setRegister(uint8_t R, int32_t V) {
  assert(R != 0 && "Trying to set $zero!\n");
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
  
  setPC(reader.get_entry());

  return 1;
}
