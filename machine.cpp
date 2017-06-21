#include <elfio/elfio.hpp>
#include <machine.hpp>

#include <cstring>
#include <cassert>

using namespace dbt;

union HalfUn {
	char asC_[2];
	uint16_t asH_;
};

void copystr(char* Target, const char* Source, uint32_t Size) {
  for (uint32_t i = 0; i < Size; ++i)
    Target[i] = Source[i];
}

void Machine::setCodeMemory(uint32_t StartAddress, uint32_t Size, const char* CodeBuffer) {
  CodeMemOffset = StartAddress;
  CodeMemory = uptr<Word[]>(new Word[Size]);
  CodeMemLimit = Size + CodeMemOffset;
  for (uint32_t i = 0; i < Size; i++) {
    Word Bytes = {CodeBuffer[i], CodeBuffer[i+1], CodeBuffer[i+2], CodeBuffer[i+3]};
    CodeMemory[i] = Bytes;
  }
}

void Machine::allocDataMemory(uint32_t Offset, uint32_t TotalSize) {
  DataMemOffset = Offset;
  DataMemLimit = Offset + TotalSize;
  DataMemory = std::unique_ptr<char[]>(new char[TotalSize]);
}

void Machine::addDataMemory(uint32_t StartAddress, uint32_t Size, const char* DataBuffer) {
  uint32_t Offset = StartAddress - DataMemOffset;
  DataMemLimit += Size;
  copystr(DataMemory.get() + Offset, DataBuffer, Size);
}

uint32_t Machine::getPC() {
  return PC;
}

uint32_t Machine::getLastPC() {
  return LastPC;
}

void Machine::incPC() {
  PC += 4;
}

void Machine::setPC(uint32_t NewPC) {
  assert((NewPC >= CodeMemOffset && NewPC < CodeMemLimit) &&
      "Jumping for an address out of border!");

  LastPC = PC;
  PC = NewPC;
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

void Machine::setMemByteAt(uint32_t Addr, uint8_t Value) {
  assert((Addr >= DataMemOffset && Addr < DataMemLimit) &&
      "Trying to access an address out of border!");

  uint32_t CorrectAddr = Addr - DataMemOffset;
  DataMemory[CorrectAddr] = Value;
}

uint8_t Machine::getMemByteAt(uint32_t Addr) {
  assert((Addr >= DataMemOffset && Addr < DataMemLimit) &&
      "Trying to access an address out of border!");

  uint32_t CorrectAddr = Addr - DataMemOffset;
  return DataMemory[CorrectAddr];
}

uint16_t Machine::getMemHalfAt(uint32_t Addr) {
  assert((Addr >= DataMemOffset && Addr < DataMemLimit) &&
      "Trying to access an address out of border!");

  uint32_t CorrectAddr = Addr - DataMemOffset;
  HalfUn Half = {DataMemory[CorrectAddr], DataMemory[CorrectAddr+1]};
  return Half.asH_;
}

Word Machine::getMemValueAt(uint32_t Addr) {
  assert((Addr >= DataMemOffset && Addr < DataMemLimit) &&
      "Trying to access an address out of border!");

  uint32_t CorrectAddr = Addr - DataMemOffset;
  Word Bytes = {DataMemory[CorrectAddr], DataMemory[CorrectAddr+1], DataMemory[CorrectAddr+2], DataMemory[CorrectAddr+3]};

  return Bytes;
}

void Machine::setMemValueAt(uint32_t Addr, uint32_t Value) {
  assert((Addr >= DataMemOffset && Addr < DataMemLimit) &&
      "Trying to access an address out of border!");

  uint32_t CorrectAddr = Addr - DataMemOffset;
  DataMemory[CorrectAddr+3] = (Value >> 24) & 0xFF;
  DataMemory[CorrectAddr+2] = (Value >> 16) & 0xFF;
  DataMemory[CorrectAddr+1] = (Value >> 8 ) & 0xFF;
  DataMemory[CorrectAddr]   = (Value      ) & 0xFF;
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

uint32_t Machine::getDataMemOffset() {
  return DataMemOffset;
}

int32_t Machine::getRegister(uint16_t R) {
  return Register[R];
}

float Machine::getFloatRegister(uint16_t R) {
  Word DW;
  DW.asI_ = Register[R + 66];
  return DW.asF_;
}

double Machine::getDoubleRegister(uint16_t R) {
  QWord QW;
  QW.asI32_[0] = Register[(R * 2) + 129 + 1];
  QW.asI32_[1] = Register[(R * 2) + 129];
  return QW.asD_;
}

void Machine::setRegister(uint16_t R, int32_t V) {
  Register[R] = V;
}

void Machine::setFloatRegister(uint16_t R, float V) {
  Word DW;
  DW.asF_ = V;
  Register[R + 66] = DW.asI_;
}

void Machine::setDoubleRegister(uint16_t R, double V) {
  QWord QW;
  QW.asD_ = V;
  Register[(R * 2) + 129]     = QW.asI32_[1];
  Register[(R * 2) + 129 + 1] = QW.asI32_[0];
}

int32_t* Machine::getRegisterPtr() {
  return Register;
}

char* Machine::getByteMemoryPtr() {
  return DataMemory.get();
}

uint32_t* Machine::getMemoryPtr() {
  return (uint32_t*) DataMemory.get();
}

using namespace ELFIO;

#define STACK_SIZE 10 * 1024 * 1024
#define HEAP_SIZE  10 * 1024 * 1024
int Machine::loadELF(const std::string ElfPath) {
  elfio reader;

  if (!reader.load(ElfPath))
    return 0;

  Elf_Half sec_num = reader.sections.size();

  uint32_t TotalDataSize = 0; 
  uint32_t AddressOffset = 0;
  bool Started = false;
  bool First = false;
  for (int i = 0; i < sec_num; ++i) {
    section* psec = reader.sections[i];

    if (Started && (psec->get_flags() & 0x2) != 0) {
      TotalDataSize += psec->get_size();
      if (!First) {
        AddressOffset = psec->get_address();
        First = true;
      }
    }

    if (psec->get_name() == ".text") 
      Started = true;
  }

  allocDataMemory(AddressOffset, (TotalDataSize + STACK_SIZE + HEAP_SIZE) + (4 - (TotalDataSize + STACK_SIZE + HEAP_SIZE) % 4));

  Started = false;
  for (int i = 0; i < sec_num; ++i) {
    section* psec = reader.sections[i];
    if (Started && (psec->get_flags() & 0x2) != 0 && psec->get_data() != nullptr) {
      addDataMemory(psec->get_address(), psec->get_size(), psec->get_data());
    }

    if (psec->get_name() == ".text") { 
      setCodeMemory(psec->get_address(), psec->get_size(),  psec->get_data());
      Started = true;
    }
  }

  setRegister(29, DataMemLimit-STACK_SIZE/4); //StackPointer
  setRegister(30, DataMemLimit-STACK_SIZE/4); //StackPointer
  
  setPC(reader.get_entry());

  return 1;
}
