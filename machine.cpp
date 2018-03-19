#include <elfio/elfio.hpp>
#include <machine.hpp>

#include <cstring>

using namespace dbt;

#ifdef DEBUG
  #define CORRECT_ASSERT() assert(Addr>=DataMemOffset && "Error on correcting address. Data memory offset Value < 0!")
#else
  #define CORRECT_ASSERT()
#endif //DEBUG

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

void Machine::setArgumentsForBin(std::string parameters)
{
  char *p;
  std::istringstream iss(parameters);
  
  std::vector<std::string> argv(std::istream_iterator<std::string>{iss}, std::istream_iterator<std::string>());
  
  setMemValueAt(getRegister(29), argv.size()+1);

//  offset = DataMemLimit - BinPath.length() + parameters.size();
  /*strcpy(&(DataMemory[DataMemLimit]), BinPath.c_str());

  for (auto argument : argv) {
    offset += argument.length()-1;
    strcpy(&(DataMemory[offset]), argument.c_str());
  }*/
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
  LastPC = PC;
  PC = NewPC;
}

Word Machine::getInstAt(uint32_t Addr) {
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
  uint32_t CorrectAddr = Addr - DataMemOffset;
  CORRECT_ASSERT();
  DataMemory[CorrectAddr] = Value;
}

uint8_t Machine::getMemByteAt(uint32_t Addr) {
  uint32_t CorrectAddr = Addr - DataMemOffset;
  CORRECT_ASSERT();
  return DataMemory[CorrectAddr];
}

uint16_t Machine::getMemHalfAt(uint32_t Addr) {
  uint32_t CorrectAddr = Addr - DataMemOffset;
  CORRECT_ASSERT();
  HalfUn Half = {DataMemory[CorrectAddr], DataMemory[CorrectAddr+1]};
  return Half.asH_;
}

Word Machine::getMemValueAt(uint32_t Addr) {
  uint32_t CorrectAddr = Addr - DataMemOffset;
  assert((Addr % 4) == 0 && "Address not aligned!");
  Word Bytes;
  CORRECT_ASSERT();
  Bytes.asI_ = *((uint32_t*)(DataMemory.get() + CorrectAddr)); 
  return Bytes;
}

void Machine::setMemValueAt(uint32_t Addr, uint32_t Value) {
  uint32_t CorrectAddr = Addr - DataMemOffset;
  assert((Addr % 4) == 0 && "Address not aligned!");
  CORRECT_ASSERT();
  *((uint32_t*)(DataMemory.get() + CorrectAddr)) = Value;
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
  return ((float*) Register)[R + 66];
}

double Machine::getDoubleRegister(uint16_t R) {
  return ((double*)Register)[R + 65];
}

void Machine::setRegister(uint16_t R, int32_t V) {
  Register[R] = V;
}

void Machine::setFloatRegister(uint16_t R, float V) {
  ((float*)Register)[R + 66] = V;
}

void Machine::setDoubleRegister(uint16_t R, double V) {
  ((double*)Register)[R + 65] = V;
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

bool Machine::isOnNativeExecution() {
  return OnNativeExecution;
}

uint32_t Machine::getRegionBeingExecuted() {
  return RegionBeingExecuted;
}

void Machine::setOnNativeExecution(uint32_t EntryRegionAddrs) {
  OnNativeExecution   = true;
  RegionBeingExecuted = EntryRegionAddrs; 
}

void Machine::setOffNativeExecution() {
  OnNativeExecution   = false;
}

bool Machine::isMethodEntry(uint32_t Addr) {
  return Symbols.count(Addr) != 0;
}

uint32_t Machine::getMethodEnd(uint32_t Addr) {
  return Symbols[Addr].second;
}

std::string Machine::getMethodName(uint32_t Addr) {
  return Symbols[Addr].first;
}

std::vector<uint32_t> Machine::getVectorOfMethodEntries() {
  std::vector<uint32_t> R;
  for (auto KV : Symbols)
    R.push_back(KV.first);
  return R;
}

using namespace ELFIO;

#define STACK_SIZE 512 * 1024 * 1024 /*512mb*/
#define HEAP_SIZE  512 * 1024 * 1024 /*512mb*/

void Machine::reset() {
  loadELF(BinPath);
}

int Machine::loadELF(const std::string ElfPath) {
  BinPath = ElfPath;

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

  std::unordered_map<uint32_t, std::string> SymbolNames;
  std::set<uint32_t> SymbolStartAddresses;

  Started = false;
  for (int i = 0; i < sec_num; ++i) {
    section* psec = reader.sections[i];
    if (Started && (psec->get_flags() & 0x2) != 0 && psec->get_data() != nullptr) {
      addDataMemory(psec->get_address(), psec->get_size(), psec->get_data());
    }

    if (psec->get_name() == ".text") { 
      setCodeMemory(psec->get_address(), psec->get_size(),  psec->get_data());
      SymbolStartAddresses.insert(psec->get_address() + psec->get_size());
      Started = true;
    }

    if (psec->get_name() == ".symtab") {
      const symbol_section_accessor symbols(reader, psec);
      std::string   name = "";
      Elf64_Addr    value = 0;
      Elf_Xword     size;
      unsigned char bind;
      unsigned char type = 0;
      Elf_Half      section_index;
      unsigned char other;
      for ( unsigned int j = 0; j < symbols.get_symbols_num(); ++j ) {
        symbols.get_symbol( j, name, value, size, bind, type, section_index, other );
        if (type == 0 && name != "" && value != 0) { 
          SymbolStartAddresses.insert(value);
          SymbolNames[value] = name;
        }
      }
    }
  }

  for (auto I = SymbolStartAddresses.begin(); I != SymbolStartAddresses.end(); ++I) 
    Symbols[*I] = {SymbolNames[*I], *SymbolStartAddresses.upper_bound(*I)};

  uint32_t StackAddr = DataMemLimit-STACK_SIZE/4;
  setRegister(29, StackAddr - (StackAddr%4)); //StackPointer
  setRegister(30, StackAddr - (StackAddr%4)); //StackPointer
  
  setPC(reader.get_entry());

  return 1;
}
