#include <interpreter.hpp>

#include <cmath>
#include <ctime>
#include <iostream>

#include <papi.h>

using namespace dbt;

bool ITDInterpreter::
isAddrsContainedIn(uint32_t StartAddrs, uint32_t EndAddrs) {
  return !(StartAddrs < LastStartAddrs || EndAddrs > LastEndAddrs);
}

constexpr uint8_t getRS(Word W) {
  return (W.asI_ >> 6) & 0x3F;
}

constexpr uint8_t getRT(Word W) {
  return W.asI_ & 0x3F;
}

constexpr uint8_t getRD(Word W) {
  return (W.asI_ >> 12) & 0x3F;
}

int16_t getImm(Word W) {
  uint16_t x = (W.asI_ >> 6) & 0x3FFF;
  return ((x >= (1 << 13))
           ? -(16384 - x) 
                     : x);
}

int16_t getImm1(Word W) {
  uint16_t x = (W.asI_ >> 12) & 0x3FFF;
  return ((x >= (1 << 13))
           ? -(16384 - x) 
                     : x);
}

int16_t getImm2(Word W) {
  uint16_t x = (W.asI_ >> 26) & 0x3FFF;
  return ((x >= (1 << 13))
           ? -(16384 - x) 
                     : x);
}

constexpr uint32_t getPL18(Word W) {
  return W.asI_ & 0x3FFFF; 
}

constexpr uint32_t getAddr(Word W) {
  return W.asI_ & 0xFFFFF;
}

constexpr uint32_t getLAddr(Word W) {
  return W.asI_ & 0x3FFFFFF;
}

int num = 0;
void* ITDInterpreter::getDispatchValue(uint32_t Addrs) {
  num++;
  return (void*)DispatchValues[(Addrs-LastStartAddrs)/4];
}

void ITDInterpreter::setDispatchValue(uint32_t Addrs, int* Target) {
  DispatchValues[(Addrs-LastStartAddrs)/4] = Target;
}

ITDInterpreter::DecodedInst ITDInterpreter::getDecodedInst(uint32_t Addrs) {
  return DecodedInsts[(Addrs-LastStartAddrs)/4];
}

void ITDInterpreter::
setDecodedInst(uint32_t Addrs, ITDInterpreter::DecodedInst DI) {
  DecodedInsts[(Addrs-LastStartAddrs)/4] = DI;
}

uint8_t ldiReg = 0;
void ITDInterpreter::
dispatch(Machine& M, uint32_t StartAddrs, uint32_t EndAddrs) {
  for (uint32_t Addrs = StartAddrs; Addrs < EndAddrs; Addrs+=4) {
    Word W = M.getInstAt(Addrs);
    uint8_t Op = W.asI_ >> 26;
    constexpr unsigned OpMask = 0x3FFFFFF;

    ITDInterpreter::DecodedInst I;

    uint8_t Ext;
    switch(Op) {
    case 0b100010:
      Ext = (W.asI_ & OpMask) >> 12;
      if (Ext == 0b101) setDispatchValue(Addrs, static_cast<int*>(&&absd));

      I.RS = getRS(W);
      I.RT = getRT(W);
      break;
    case 0b100000:
      Ext = W.asI_ >> 18;
      if (Ext == 0b0) setDispatchValue(Addrs, static_cast<int*>(&&add));
      if (Ext == 0b1) {
        setDispatchValue(Addrs, static_cast<int*>(&&ldihi));
        I.Addrs = getPL18(W);
      }
      if (Ext == 0b110) setDispatchValue(Addrs, static_cast<int*>(&&and_));
      if (Ext == 0b111) setDispatchValue(Addrs, static_cast<int*>(&&or_));

      I.RS = getRS(W);
      I.RT = getRT(W);
      I.RD = getRD(W);
      break;
    case 0b11111: 
      Ext = (W.asI_ & OpMask) >> 20;
      if (Ext == 0b100) setDispatchValue(Addrs, static_cast<int*>(&&ldi));

      I.RT = getRT(W);
      I.Imm = getImm(W);
      break;
    case 0b110:
      setDispatchValue(Addrs, static_cast<int*>(&&ldw));

      I.RS = getRS(W);
      I.RT = getRT(W);
      I.Imm = getImm1(W);
      break;
    case 0b1110:
      setDispatchValue(Addrs, static_cast<int*>(&&addi));
      
      I.RT = getRT(W);
      I.RS = getRS(W);
      I.Imm = getImm1(W);
      break;
    case 0b1:
      setDispatchValue(Addrs, static_cast<int*>(&&call));

      I.Addrs = getAddr(W);
      break;
    case 0b100011:
      setDispatchValue(Addrs, static_cast<int*>(&&jumpr));

      I.RT = getRT(W);
      break;
    case 0b1011:
      setDispatchValue(Addrs, static_cast<int*>(&&stw));

      I.RS = getRS(W);
      I.RT = getRT(W);
      I.Imm = getImm1(W);
      break;
    case 0b10001:
      setDispatchValue(Addrs, static_cast<int*>(&&sltiu));

      I.RS = getRS(W);
      I.RT = getRT(W);
      I.Imm = getImm2(W);
      break;
    case 0b10000:
      setDispatchValue(Addrs, static_cast<int*>(&&slti));

      I.RS = getRS(W);
      I.RT = getRT(W);
      I.Imm = getImm2(W);
      break;
    case 0b10101:
      setDispatchValue(Addrs, static_cast<int*>(&&jeq));

      I.RS = getRS(W);
      I.RT = getRT(W);
      I.Imm = getImm1(W);
      break;
    case 0b10110:
      setDispatchValue(Addrs, static_cast<int*>(&&jne));

      I.RS = getRS(W);
      I.RT = getRT(W);
      I.Imm = getImm1(W);
      break;
    case 0b0:
      setDispatchValue(Addrs, static_cast<int*>(&&jump));

      I.Addrs = getLAddr(W);
      break;
    case 0b100100:
      setDispatchValue(Addrs, static_cast<int*>(&&syscall));
      break;
    default:
      setDispatchValue(Addrs, static_cast<int*>(&&nop));
    }

    setDecodedInst(Addrs, I);
  }

  register Word W;
  ITDInterpreter::DecodedInst I;

  goto *getDispatchValue(M.getPC());

nop:
  {
  M.incPC();
  goto *getDispatchValue(M.getPC());
  }

absd:
  {
  double dtmp1;
  I = getDecodedInst(M.getPC());
  dtmp1 = M.getDoubleRegister(I.RS);
  M.setDoubleRegister(I.RT, fabs(dtmp1));
  M.incPC();
  goto *getDispatchValue(M.getPC());
  }

add:
  {
  int tmp1, tmp2;
  I = getDecodedInst(M.getPC());
  tmp1 = M.getRegister(I.RS);
  tmp2 = M.getRegister(I.RT);
  M.setRegister(I.RD, tmp1+tmp2);
  M.incPC();
  goto *getDispatchValue(M.getPC());
  }

ldi:
  {
  I = getDecodedInst(M.getPC());
  ldiReg = I.RT;
  M.setRegister(ldiReg, M.getRegister(I.RT) & 0xFFFFC000);
  M.setRegister(ldiReg, M.getRegister(ldiReg) | (I.Imm & 0x3FFF));
  M.incPC();
  goto *getDispatchValue(M.getPC());
  }

ldihi:
  {
  I = getDecodedInst(M.getPC());
  M.setRegister(ldiReg, M.getRegister(I.RT) & 0x3FFF);
  M.setRegister(ldiReg, M.getRegister(ldiReg) | (I.Addrs << 14));
  M.incPC();
  goto *getDispatchValue(M.getPC());
  }

ldw:
  {
  int tmp1;
  I = getDecodedInst(M.getPC());
  tmp1 = M.getRegister(I.RS) + I.Imm;
  M.setRegister(I.RT, M.getMemValueAt(tmp1).asI_);
  M.incPC();
  goto *getDispatchValue(M.getPC());
  }

addi:
  {
  I = getDecodedInst(M.getPC());
  M.setRegister(I.RT, M.getRegister(I.RS) + I.Imm);
  M.incPC();
  goto *getDispatchValue(M.getPC());
  }

and_:
  {
  I = getDecodedInst(M.getPC());
  M.setRegister(I.RT, I.RS & I.RD);
  M.incPC();
  goto *getDispatchValue(M.getPC());
  }

or_:
  {
  I = getDecodedInst(M.getPC());
  M.setRegister(I.RT, I.RS | I.RD);
  M.incPC();
  goto *getDispatchValue(M.getPC());
  }

call:
  {
  I = getDecodedInst(M.getPC());
  M.setRegister(31, M.getPC()+4);
  M.setPC((M.getPC() & 0xF0000000) | (I.Addrs << 2));
  goto *getDispatchValue(M.getPC());
  }

jumpr:
  {
  I = getDecodedInst(M.getPC());
  M.setPC(M.getRegister(I.RT));
  goto *getDispatchValue(M.getPC());
  }

stw:
  {
  I = getDecodedInst(M.getPC());
  M.setMemValueAt(M.getRegister(I.RS) + I.Imm, M.getRegister(I.RT));
  M.incPC();
  goto *getDispatchValue(M.getPC());
  }

sltiu:
  {
  I = getDecodedInst(M.getPC());
  if (M.getRegister(I.RS) < (I.Imm & 0x3FFF))
    M.setRegister(I.RT, 1);
  else 
    M.setRegister(I.RT, 0);
  M.incPC();
  goto *getDispatchValue(M.getPC());
  }

slti:
  {
  I = getDecodedInst(M.getPC());
  if ((int32_t)M.getRegister(I.RS) < (int32_t)(I.Imm & 0x3FFF))
    M.setRegister(I.RT, 1);
  else 
    M.setRegister(I.RT, 0);
  M.incPC();
  goto *getDispatchValue(M.getPC());
  }

jeq:
  {
  I = getDecodedInst(M.getPC());
  if (M.getRegister(I.RT) == M.getRegister(I.RS))
    M.setPC(M.getPC() + (I.Imm << 2) + 4);
  else
    M.incPC();
  goto *getDispatchValue(M.getPC());
  }

/*jeqz:
  {
  I = getDecodedInst(M.getPC());
  if (M.getRegister(I.RT == 0)
    M.setPC(M.getPC() + (I.Imm << 2));
  else
    M.incPC();
  goto *getDispatchValue(M.getPC());
  }*/

jne:
  {
  I = getDecodedInst(M.getPC());
  if (M.getRegister(I.RT) != M.getRegister(I.RS))
    M.setPC(M.getPC() + ((I.Imm << 2) + 4));
  else
    M.incPC();
  goto *getDispatchValue(M.getPC());
  }


syscall:
  return;
  goto *getDispatchValue(M.getPC());

jump:
  {
  I = getDecodedInst(M.getPC());
  M.setPC((M.getPC() & 0xF0000000) | (I.Addrs << 2));
  goto *getDispatchValue(M.getPC());
  }
}

void ITDInterpreter::
execute(Machine& M, uint32_t StartAddrs, uint32_t EndAddrs) {
  if (!DispatchValues || !isAddrsContainedIn(StartAddrs, EndAddrs)) { 
    DispatchValues = new int*[(EndAddrs - StartAddrs)/4];
    DecodedInsts = new DecodedInst[(EndAddrs - StartAddrs)/4];
  }

  LastStartAddrs = StartAddrs;
  LastEndAddrs = EndAddrs;

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
