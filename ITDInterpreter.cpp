#include <interpreter.hpp>

#include <cmath>
#include <ctime>
#include <iostream>

#include <papi.h>

bool dbt::ITDInterpreter::
isAddrsContainedIn(uint32_t StartAddrs, uint32_t EndAddrs) {
  return !(StartAddrs < LastStartAddrs || EndAddrs > LastEndAddrs);
}

constexpr uint8_t getRS(dbt::Word W) {
  return (W.asI_ >> 6) & 0x3F;
}

constexpr uint8_t getRT(dbt::Word W) {
  return W.asI_ & 0x3F;
}

constexpr uint8_t getRD(dbt::Word W) {
  return (W.asI_ >> 12) & 0x3F;
}

int16_t getImm(dbt::Word W) {
  uint16_t x = (W.asI_ >> 6) & 0x3FFF;
  return ((x >= (1 << 13))
           ? -(16384 - x) 
                     : x);
}

int16_t getImm1(dbt::Word W) {
  uint16_t x = (W.asI_ >> 12) & 0x3FFF;
  return ((x >= (1 << 13))
           ? -(16384 - x) 
                     : x);
}

int16_t getImm2(dbt::Word W) {
  uint16_t x = (W.asI_ >> 26) & 0x3FFF;
  return ((x >= (1 << 13))
           ? -(16384 - x) 
                     : x);
}

constexpr uint32_t getPL18(dbt::Word W) {
  return W.asI_ & 0x3FFFF; 
}

constexpr uint32_t getAddr(dbt::Word W) {
  return W.asI_ & 0xFFFFF;
}

constexpr uint32_t getLAddr(dbt::Word W) {
  return W.asI_ & 0x3FFFFFF;
}

int num = 0;
void* dbt::ITDInterpreter::getDispatchValue(uint32_t Addrs) {
  num++;
  return (void*)DispatchValues[(Addrs-LastStartAddrs)/4];
}

void dbt::ITDInterpreter::setDispatchValue(uint32_t Addrs, int* Target) {
  DispatchValues[(Addrs-LastStartAddrs)/4] = Target;
}

uint8_t ldiReg = 0;

void dbt::ITDInterpreter::
dispatch(Machine& M, uint32_t StartAddrs, uint32_t EndAddrs) {
  for (uint32_t Addrs = StartAddrs; Addrs < EndAddrs; Addrs+=4) {
    dbt::Word W = M.getInstAt(Addrs);
    uint8_t Op = W.asI_ >> 26;
    constexpr unsigned OpMask = 0x3FFFFFF;

    uint8_t Ext;
    switch(Op) {
    case 0b100010:
      Ext = (W.asI_ & OpMask) >> 12;
      if (Ext == 0b101) setDispatchValue(Addrs, static_cast<int*>(&&absd));
      break;
    case 0b100000:
      Ext = W.asI_ >> 18;
      if (Ext == 0b0) setDispatchValue(Addrs, static_cast<int*>(&&add));
      if (Ext == 0b1) setDispatchValue(Addrs, static_cast<int*>(&&ldihi));
      if (Ext == 0b110) setDispatchValue(Addrs, static_cast<int*>(&&and_));
      if (Ext == 0b111) setDispatchValue(Addrs, static_cast<int*>(&&or_));
      break;
    case 0b11111: 
      Ext = (W.asI_ & OpMask) >> 20;
      if (Ext == 0b100) setDispatchValue(Addrs, static_cast<int*>(&&ldi));
      break;
    case 0b110:
      setDispatchValue(Addrs, static_cast<int*>(&&ldw));
      break;
    case 0b1110:
      setDispatchValue(Addrs, static_cast<int*>(&&addi));
      break;
    case 0b1:
      setDispatchValue(Addrs, static_cast<int*>(&&call));
      break;
    case 0b100011:
      setDispatchValue(Addrs, static_cast<int*>(&&jumpr));
      break;
    case 0b1011:
      setDispatchValue(Addrs, static_cast<int*>(&&stw));
      break;
    case 0b10001:
      setDispatchValue(Addrs, static_cast<int*>(&&sltiu));
      break;
    case 0b10000:
      setDispatchValue(Addrs, static_cast<int*>(&&slti));
      break;
    case 0b10101:
      setDispatchValue(Addrs, static_cast<int*>(&&jeq));
      break;
    case 0b10110:
      setDispatchValue(Addrs, static_cast<int*>(&&jne));
      break;
    case 0b0:
      setDispatchValue(Addrs, static_cast<int*>(&&jump));
      break;
    case 0b100100:
      setDispatchValue(Addrs, static_cast<int*>(&&syscall));
      break;
    default:
      setDispatchValue(Addrs, static_cast<int*>(&&nop));
    }
  }

  register dbt::Word W;

  goto *getDispatchValue(M.getPC());

nop:
  {
  W = M.getInstAtPC();
  M.incPC();
  goto *getDispatchValue(M.getPC());
  }

absd:
  {
  double dtmp1;
  W = M.getInstAtPC();
  dtmp1 = M.getDoubleRegister(getRS(W));
  M.setDoubleRegister(getRT(W), fabs(dtmp1));
  M.incPC();
  goto *getDispatchValue(M.getPC());
  }

add:
  {
  int tmp1, tmp2;
  W = M.getInstAtPC();
  tmp1 = M.getRegister(getRS(W));
  tmp2 = M.getRegister(getRT(W));
  M.setRegister(getRD(W), tmp1+tmp2);
  M.incPC();
  goto *getDispatchValue(M.getPC());
  }

ldi:
  {
  W = M.getInstAtPC();
  ldiReg = getRT(W);
  M.setRegister(ldiReg, M.getRegister(getRT(W)) & 0xFFFFC000);
  M.setRegister(ldiReg, M.getRegister(ldiReg) | (getImm(W) & 0x3FFF));
  M.incPC();
  goto *getDispatchValue(M.getPC());
  }

ldihi:
  {
  W = M.getInstAtPC();
  M.setRegister(ldiReg, M.getRegister(getRT(W)) & 0x3FFF);
  M.setRegister(ldiReg, M.getRegister(ldiReg) | (getPL18(W) << 14));
  M.incPC();
  goto *getDispatchValue(M.getPC());
  }

ldw:
  {
  int tmp1;
  W = M.getInstAtPC();
  tmp1 = M.getRegister(getRS(W)) + getImm1(W);
  M.setRegister(getRT(W), M.getMemValueAt(tmp1).asI_);
  M.incPC();
  goto *getDispatchValue(M.getPC());
  }

addi:
  {
  W = M.getInstAtPC();
  M.setRegister(getRT(W), M.getRegister(getRS(W)) + getImm1(W));
  M.incPC();
  goto *getDispatchValue(M.getPC());
  }

and_:
  {
  W = M.getInstAtPC();
  M.setRegister(getRT(W), getRS(W) & getRD(W));
  M.incPC();
  goto *getDispatchValue(M.getPC());
  }

call:
  {
  W = M.getInstAtPC();
  M.setRegister(31, M.getPC()+4);
  M.setPC((M.getPC() & 0xF0000000) | (getAddr(W) << 2));
  goto *getDispatchValue(M.getPC());
  }

jumpr:
  {
  W = M.getInstAtPC();
  M.setPC(M.getRegister(getRT(W)));
  goto *getDispatchValue(M.getPC());
  }

stw:
  {
  W = M.getInstAtPC(); 
  M.setMemValueAt(M.getRegister(getRS(W)) + getImm1(W), M.getRegister(getRT(W)));
  M.incPC();
  goto *getDispatchValue(M.getPC());
  }

sltiu:
  {
  W = M.getInstAtPC();
  if (M.getRegister(getRS(W)) < (getImm2(W) & 0x3FFF))
    M.setRegister(getRT(W), 1);
  else 
    M.setRegister(getRT(W), 0);
  M.incPC();
  goto *getDispatchValue(M.getPC());
  }

slti:
  {
  W = M.getInstAtPC();
  if ((int32_t)M.getRegister(getRS(W)) < (int32_t)(getImm2(W) & 0x3FFF))
    M.setRegister(getRT(W), 1);
  else 
    M.setRegister(getRT(W), 0);
  M.incPC();
  goto *getDispatchValue(M.getPC());
  }

jeq:
  {
  W = M.getInstAtPC();
  if (M.getRegister(getRT(W)) == M.getRegister(getRS(W)))
    M.setPC(M.getPC() + (getImm1(W) << 2) + 4);
  else
    M.incPC();
  goto *getDispatchValue(M.getPC());
  }

/*jeqz:
  {
  W = M.getInstAtPC(); 
  if (M.getRegister(getRT(W)) == 0)
    M.setPC(M.getPC() + (getImm(W) << 2));
  else
    M.incPC();
  goto *getDispatchValue(M.getPC());
  }*/

jne:
  {
  W = M.getInstAtPC();
  if (M.getRegister(getRT(W)) != M.getRegister(getRS(W)))
    M.setPC(M.getPC() + ((getImm1(W) << 2) + 4));
  else
    M.incPC();
  goto *getDispatchValue(M.getPC());
  }

or_:
  {
  W = M.getInstAtPC();
  M.setRegister(getRT(W), getRS(W) | getRD(W));
  M.incPC();
  goto *getDispatchValue(M.getPC());
  }

syscall:
  return;
  goto *getDispatchValue(M.getPC());

jump:
  {
  W = M.getInstAtPC();
  M.setPC((M.getPC() & 0xF0000000) | (getLAddr(W) << 2));
  goto *getDispatchValue(M.getPC());
  }
}

void dbt::ITDInterpreter::
execute(Machine& M, uint32_t StartAddrs, uint32_t EndAddrs) {
  if (!DispatchValues || !isAddrsContainedIn(StartAddrs, EndAddrs)) { 
    DispatchValues = new int*[(EndAddrs - StartAddrs)/4];
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
            << "Misspredicted Branches: " << (double)values[4]/values[3] << std::endl
            << "Total L2 Cache Misses: " << (double)values[0] << std::endl;
}
