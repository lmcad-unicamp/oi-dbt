#include <interpreter.hpp>
#include <OIDecoder.hpp>

#include <cmath>
#include <ctime>
#include <iostream>

#include <papi.h>

using namespace dbt;
using namespace dbt::OIDecoder;

#ifdef NDEBUG
#define DEBUG_PRINT(Addr, Inst)
#else
#include <OIPrinter.hpp>
#define DEBUG_PRINT(Addr, Inst) std::cout << std::hex << Addr << "\t" << OIPrinter::getString(Inst) << std::dec << "\n";
#endif

bool ITDInterpreter::
isAddrsContainedIn(uint32_t StartAddrs, uint32_t EndAddrs) {
  return !(StartAddrs < LastStartAddrs || EndAddrs > LastEndAddrs);
}

int num = 0;
void* ITDInterpreter::getDispatchValue(uint32_t Addrs) {
  num++;
  return (void*)DispatchValues[(Addrs-LastStartAddrs)/4];
}

void ITDInterpreter::setDispatchValue(uint32_t Addrs, int* Target) {
  DispatchValues[(Addrs-LastStartAddrs)/4] = Target;
}

OIInst ITDInterpreter::getDecodedInst(uint32_t Addrs) {
  return DecodedInsts[(Addrs-LastStartAddrs)/4];
}

void ITDInterpreter::
setDecodedInst(uint32_t Addrs, OIInst DI) {
  DecodedInsts[(Addrs-LastStartAddrs)/4] = DI;
}

uint8_t ldiReg = 0;
void ITDInterpreter::
dispatch(Machine& M, uint32_t StartAddrs, uint32_t EndAddrs) {
  for (uint32_t Addrs = StartAddrs; Addrs < EndAddrs; Addrs+=4) {
    Word W = M.getInstAt(Addrs);

    OIInst I = decode(W.asI_);

    switch(I.Type) {
    case OIInstType::Absd:
      setDispatchValue(Addrs, static_cast<int*>(&&absd));
      break;
    case Add:
      setDispatchValue(Addrs, static_cast<int*>(&&add));
      break;
    case Sub:
      setDispatchValue(Addrs, static_cast<int*>(&&sub));
      break;
    case Ldihi:
      setDispatchValue(Addrs, static_cast<int*>(&&ldihi));
      break;
    case And: 
      setDispatchValue(Addrs, static_cast<int*>(&&and_));
      break;
    case Or: 
      setDispatchValue(Addrs, static_cast<int*>(&&or_));
      break;
    case Ldi:
      setDispatchValue(Addrs, static_cast<int*>(&&ldi));
      break;
    case Ldw:
      setDispatchValue(Addrs, static_cast<int*>(&&ldw));
      break;
    case Addi:
      setDispatchValue(Addrs, static_cast<int*>(&&addi));
      break;
    case Call:
      setDispatchValue(Addrs, static_cast<int*>(&&call));
      break;
    case Jumpr:
      setDispatchValue(Addrs, static_cast<int*>(&&jumpr));
      break;
    case Stw:
      setDispatchValue(Addrs, static_cast<int*>(&&stw));
      break;
    case Sltiu:
      setDispatchValue(Addrs, static_cast<int*>(&&sltiu));
      break;
    case Slti:
      setDispatchValue(Addrs, static_cast<int*>(&&slti));
      break;
    case Slt:
      setDispatchValue(Addrs, static_cast<int*>(&&slt));
      break;
    case Jeq:
      setDispatchValue(Addrs, static_cast<int*>(&&jeq));
      break;
    case Jeqz:
      setDispatchValue(Addrs, static_cast<int*>(&&jeqz));
      break;
    case Jne:
      setDispatchValue(Addrs, static_cast<int*>(&&jne));
      break;
    case Jump:
      setDispatchValue(Addrs, static_cast<int*>(&&jump));
      break;
    case Mul:
      setDispatchValue(Addrs, static_cast<int*>(&&mul));
      break;
    case Div:
      setDispatchValue(Addrs, static_cast<int*>(&&div));
      break;
    case Mod:
      setDispatchValue(Addrs, static_cast<int*>(&&mod));
      break;
    case Syscall:
      setDispatchValue(Addrs, static_cast<int*>(&&syscall));
      break;
    case Shr:
      setDispatchValue(Addrs, static_cast<int*>(&&shr));
      break;
    case Shl:
      setDispatchValue(Addrs, static_cast<int*>(&&shl));
      break;
    case Nop:
      setDispatchValue(Addrs, static_cast<int*>(&&nop));
      break;
    case Null:
      exit(1);
    }

    setDecodedInst(Addrs, I);
  }

  OIInst I;
  goto next;

nop:
  {
    M.incPC();
    goto next;
  }

absd:
  {
    I = getDecodedInst(M.getPC());
    M.setDoubleRegister(I.RT, fabs(M.getDoubleRegister(I.RS)));
    M.incPC();
    goto next;
  }

add:
  {
    I = getDecodedInst(M.getPC());
    M.setRegister(I.RD, M.getRegister(I.RS)+M.getRegister(I.RT));
    M.incPC();
    goto next;
  }

sub:
  {
    I = getDecodedInst(M.getPC());
    M.setRegister(I.RD, M.getRegister(I.RS)-M.getRegister(I.RT));
    M.incPC();
    goto next;
  }

mul:
  {
    I = getDecodedInst(M.getPC());
    int64_t Result = M.getRegister(I.RS) * M.getRegister(I.RT);
    if (I.RD != 0)
      M.setRegister(I.RD, (Result & 0xFFFFFFFF));
    if (I.RV != 0)
      M.setRegister(I.RV, ((Result >> 32) & 0xFFFFFFFF));
    M.incPC();
    goto next;
  }

div:
  {
    I = getDecodedInst(M.getPC());
    M.setRegister(I.RD, M.getRegister(I.RS) / M.getRegister(I.RT));
    M.incPC();
    goto next;
  }

mod:
  {
    I = getDecodedInst(M.getPC());
    M.setRegister(I.RV, M.getRegister(I.RS) % M.getRegister(I.RT));
    M.incPC();
    goto next;
  }

ldi:
  {
    I = getDecodedInst(M.getPC());
    ldiReg = I.RT;
    M.setRegister(ldiReg, M.getRegister(I.RT) & 0xFFFFC000);
    M.setRegister(ldiReg, M.getRegister(ldiReg) | (I.Imm & 0x3FFF));
    M.incPC();
    goto next;
  }

ldihi:
  {
    I = getDecodedInst(M.getPC());
    M.setRegister(ldiReg, M.getRegister(I.RT) & 0x3FFF);
    M.setRegister(ldiReg, M.getRegister(ldiReg) | (I.Addrs << 14));
    M.incPC();
    goto next;
  }

ldw:
  {
    I = getDecodedInst(M.getPC());
    M.setRegister(I.RT, M.getMemValueAt(M.getRegister(I.RS) + I.Imm).asI_);
    M.incPC();
    goto next;
  }

addi:
  {
    I = getDecodedInst(M.getPC());
    M.setRegister(I.RT, M.getRegister(I.RS) + I.Imm);
    M.incPC();
    goto next;
  }

and_:
  {
    I = getDecodedInst(M.getPC());
    M.setRegister(I.RD, M.getRegister(I.RS) & M.getRegister(I.RT));
    M.incPC();
    goto next;
  }

or_:
  {
    I = getDecodedInst(M.getPC());
    M.setRegister(I.RD, M.getRegister(I.RS) | M.getRegister(I.RT));
    M.incPC();
    goto next;
  }

shr:
  I = getDecodedInst(M.getPC());
  M.setRegister(I.RD, M.getRegister(I.RT) >> I.RS);
  M.incPC();
  goto next;

shl:
  I = getDecodedInst(M.getPC());
  M.setRegister(I.RD, M.getRegister(I.RT) << I.RS);
  M.incPC();
  goto next;

call:
  {
    I = getDecodedInst(M.getPC());
    M.setRegister(31, M.getPC()+4);
    M.setPC((M.getPC() & 0xF0000000) | (I.Addrs << 2));
    goto next;
  }

jumpr:
  {
    I = getDecodedInst(M.getPC());
    M.setPC(M.getRegister(I.RT));
    goto next;
  }

stw:
  {
    I = getDecodedInst(M.getPC());
    M.setMemValueAt(M.getRegister(I.RS) + I.Imm, M.getRegister(I.RT));
    M.incPC();
    goto next;
  }

sltiu:
  {
    I = getDecodedInst(M.getPC());
    if (M.getRegister(I.RS) < (I.Imm & 0x3FFF))
      M.setRegister(I.RT, 1);
    else
      M.setRegister(I.RT, 0);
    M.incPC();
    goto next;
  }

slti:
  {
    I = getDecodedInst(M.getPC());
    if ((int32_t) M.getRegister(I.RS) < (int32_t) I.Imm)
      M.setRegister(I.RT, 1);
    else
      M.setRegister(I.RT, 0);
    M.incPC();
    goto next;
  }

slt:
  {
    I = getDecodedInst(M.getPC());
    if (M.getRegister(I.RS) < M.getRegister(I.RT))
      M.setRegister(I.RD, 1);
    else
      M.setRegister(I.RD, 0);
    M.incPC();
    goto next;
  }

jeq:
  {
    I = getDecodedInst(M.getPC());
    if (M.getRegister(I.RT) == M.getRegister(I.RS))
      M.setPC(M.getPC() + (I.Imm << 2));
    M.incPC();
    goto next;
  }

jeqz:
  {
    I = getDecodedInst(M.getPC());
    if (M.getRegister(I.RS) == 0)
      M.setPC(M.getPC() + (I.Imm << 2));
    M.incPC();
    goto next;
  }

jne:
  {
    I = getDecodedInst(M.getPC());
    if (M.getRegister(I.RT) != M.getRegister(I.RS))
      M.setPC(M.getPC() + ((I.Imm << 2)));
    M.incPC();
    goto next;
  }

syscall:
  if (SyscallM.processSyscall(M))
    return;
  goto next;

jump:
  {
    I = getDecodedInst(M.getPC());
    M.setPC((M.getPC() & 0xF0000000) | (I.Addrs << 2));
    goto next;
  }

next:
    DEBUG_PRINT(M.getPC(), I);
    goto *getDispatchValue(M.getPC());
}

void ITDInterpreter::
execute(Machine& M, uint32_t StartAddrs, uint32_t EndAddrs) {
  if (DispatchValues.size() == 0 || !isAddrsContainedIn(StartAddrs, EndAddrs)) { 
    DispatchValues.reserve((EndAddrs - StartAddrs)/4);
    DecodedInsts.reserve((EndAddrs - StartAddrs)/4);
  }

  LastStartAddrs = StartAddrs;
  LastEndAddrs   = EndAddrs;

  int events[5] = {PAPI_L2_TCM, PAPI_TOT_INS, PAPI_TOT_CYC, PAPI_BR_CN, PAPI_BR_MSP}, ret;
  long_long values[5];

  if ((ret = PAPI_start_counters(events, 5)) != PAPI_OK) {
    fprintf(stderr, "PAPI failed to start counters: %s\n", PAPI_strerror(ret));
    exit(1);
  }

  dispatch(M, StartAddrs, EndAddrs);

  if ((ret = PAPI_read_counters(values, 5)) != PAPI_OK) {
    fprintf(stderr, "PAPI failed to read counters: %s\n", PAPI_strerror(ret));
    exit(1);
  }

  std::cout << "Number of Instructions Emulated: " << num << std::endl 
    << "Number of Native Instructions Executed:" << (double)values[1] << std::endl
    << "Native/Emulated Proportion: " << (double)values[1]/num << std::endl
    << "Clock/Emulated: " << (double)values[2]/num << std::endl
    << "Misspredicted Branches: " << (double)values[4] << std::endl
    << "Total L2 Cache Misses: " << (double)values[0] << std::endl;
}
