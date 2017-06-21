#include <OIDecoder.hpp>
#include <interpreter.hpp>
#include <timer.hpp>

#include <cmath>
#include <ctime>
#include <iostream>

using namespace dbt;
using namespace dbt::OIDecoder;
unsigned total = 0;

#define LDI_REG   64
#define IJMP_REG  65
#define CC_REG    257

#include <OIPrinter.hpp>
#define DEBUG_PRINT(Addr, Inst) std::cout << std::hex << Addr << "\t" << OIPrinter::getString(Inst) << std::dec << "\n";

#define SET_DISPACH(Addrs, Label, Offset)\
  case Label:\
    setDispatchValue(Addrs, static_cast<int*>(Offset));\
    break;

#define GOTO_NEXT\
    goto *getDispatchValue(M.getPC());

#define IMPLEMENT(Label, Code)\
  Label:\
    I = getDecodedInst(M.getPC());\
    {\
       Code\
    }\
    M.incPC();\
    GOTO_NEXT

#define IMPLEMENT_JMP(Label, Code)\
  Label:\
    I = getDecodedInst(M.getPC());\
    {\
      Code\
      ImplRFT.onBranch(M);\
    }\
    GOTO_NEXT

#define IMPLEMENT_BR(Label, Code)\
  Label:\
    I = getDecodedInst(M.getPC());\
    {\
      Code\
    }\
    M.incPC();\
    GOTO_NEXT

bool isnan(double x) { return x != x; }
bool isnan(float x)  { return x != x; }

bool ITDInterpreter::isAddrsContainedIn(uint32_t StartAddrs, uint32_t EndAddrs) {
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

void ITDInterpreter::setDecodedInst(uint32_t Addrs, OIInst DI) {
  DecodedInsts[(Addrs-LastStartAddrs)/4] = DI;
}

void ITDInterpreter::dispatch(Machine& M, uint32_t StartAddrs, uint32_t EndAddrs) {
  for (uint32_t Addrs = StartAddrs; Addrs < EndAddrs; Addrs+=4) {
    Word W = M.getInstAt(Addrs);
    OIInst I = decode(W.asI_);
    switch(I.Type) {
      SET_DISPACH(Addrs, Add,     &&add);
      SET_DISPACH(Addrs, Sub,     &&sub);
      SET_DISPACH(Addrs, Ldihi,   &&ldihi);
      SET_DISPACH(Addrs, And,     &&and_);
      SET_DISPACH(Addrs, Andi,    &&andi);
      SET_DISPACH(Addrs, Or,      &&or_);
      SET_DISPACH(Addrs, Nor,     &&nor);
      SET_DISPACH(Addrs, Ldh,     &&ldh);
      SET_DISPACH(Addrs, Ldi,     &&ldi);
      SET_DISPACH(Addrs, Ldw,     &&ldw);
      SET_DISPACH(Addrs, Addi,    &&addi);
      SET_DISPACH(Addrs, Call,    &&call);
      SET_DISPACH(Addrs, Callr,   &&callr);
      SET_DISPACH(Addrs, Jumpr,   &&jumpr);
      SET_DISPACH(Addrs, Stw,     &&stw);
      SET_DISPACH(Addrs, Sltiu,   &&sltiu);
      SET_DISPACH(Addrs, Slti,    &&slti);
      SET_DISPACH(Addrs, Sltu,    &&sltu);
      SET_DISPACH(Addrs, Slt,     &&slt);
      SET_DISPACH(Addrs, Jeq,     &&jeq);
      SET_DISPACH(Addrs, Jeqz,    &&jeqz);
      SET_DISPACH(Addrs, Jgtz,    &&jgtz);
      SET_DISPACH(Addrs, Jgez,    &&jgez);
      SET_DISPACH(Addrs, Jlez,    &&jlez);
      SET_DISPACH(Addrs, Jltz,    &&jltz);
      SET_DISPACH(Addrs, Jne,     &&jne);
      SET_DISPACH(Addrs, Jnez,    &&jnez);
      SET_DISPACH(Addrs, Jump,    &&jump);
      SET_DISPACH(Addrs, Mul,     &&mul);
      SET_DISPACH(Addrs, Mulu,    &&mulu);
      SET_DISPACH(Addrs, Div,     &&div);
      SET_DISPACH(Addrs, Mod,     &&mod);
      SET_DISPACH(Addrs, Divu,    &&divu);
      SET_DISPACH(Addrs, Modu,    &&modu);
      SET_DISPACH(Addrs, Syscall, &&syscall);
      SET_DISPACH(Addrs, Shr,     &&shr);
      SET_DISPACH(Addrs, Asr,     &&asr);
      SET_DISPACH(Addrs, Shl,     &&shl);
      SET_DISPACH(Addrs, Shlr,    &&shlr);
      SET_DISPACH(Addrs, Movn,    &&movn);
      SET_DISPACH(Addrs, Movz,    &&movz);
      SET_DISPACH(Addrs, Ori,     &&ori);
      SET_DISPACH(Addrs, Xori,    &&xori);
      SET_DISPACH(Addrs, Xor,     &&xor_);
      SET_DISPACH(Addrs, Stb,     &&stb);
      SET_DISPACH(Addrs, Ldb,     &&ldb);
      SET_DISPACH(Addrs, Ldbu,    &&ldbu);
      SET_DISPACH(Addrs, Ldhu,    &&ldhu);
      SET_DISPACH(Addrs, Sth,     &&sth);
      SET_DISPACH(Addrs, Seh,     &&seh);
      SET_DISPACH(Addrs, Seb,     &&seb);
      SET_DISPACH(Addrs, Ijmphi,  &&ijmphi);
      SET_DISPACH(Addrs, Ijmp,    &&ijmp);
      SET_DISPACH(Addrs, Ldc1,    &&ldc1);
      SET_DISPACH(Addrs, Sdc1,    &&sdc1);
      SET_DISPACH(Addrs, Mtlc1,   &&mtlc1);
      SET_DISPACH(Addrs, Mthc1,   &&mthc1);
      SET_DISPACH(Addrs, Ceqs,    &&ceqs);
      SET_DISPACH(Addrs, Ceqd,    &&ceqd);
      SET_DISPACH(Addrs, Bc1f,    &&bc1f);
      SET_DISPACH(Addrs, Bc1t,    &&bc1t);
      SET_DISPACH(Addrs, Movd,    &&movd);
      SET_DISPACH(Addrs, Lwc1,    &&lwc1);
      SET_DISPACH(Addrs, Adds,    &&adds);
      SET_DISPACH(Addrs, Mtc1,    &&mtc1);
      SET_DISPACH(Addrs, Mfc1,    &&mfc1);
      SET_DISPACH(Addrs, Truncws, &&truncws);
      SET_DISPACH(Addrs, Cvtsw,   &&cvtsw);
      SET_DISPACH(Addrs, Nop,     &&nop);
      case Null:
        exit(1);
    }
    setDecodedInst(Addrs, I);
  }

  // ---------------------------------------- Trampoline Zone ------------------------------------------ //

  OIInst I;
  GOTO_NEXT;

  IMPLEMENT(nop, );
  
  /**********************   Int Inst   **************************/

  IMPLEMENT(add, 
      M.setRegister(I.RD, M.getRegister(I.RS)+M.getRegister(I.RT));
    );
  
  IMPLEMENT(sub, 
      M.setRegister(I.RD, M.getRegister(I.RS)-M.getRegister(I.RT));
    );
  
  IMPLEMENT(mul, 
      int64_t Result = (int64_t) M.getRegister(I.RS) * (int64_t) M.getRegister(I.RT);
      if (I.RD != 0)
        M.setRegister(I.RD, (Result & 0xFFFFFFFF));
      if (I.RV != 0) 
        M.setRegister(I.RV, ((Result >> 32) & 0xFFFFFFFF));
    );
  
  IMPLEMENT(mulu, 
      uint64_t Result = (uint64_t) M.getRegister(I.RS) * (uint64_t) M.getRegister(I.RT);
      if (I.RD != 0)  
        M.setRegister(I.RD, (Result & 0xFFFFFFFF));
      if (I.RV != 0) 
        M.setRegister(I.RV, ((Result >> 32) & 0xFFFFFFFF));
    );
  
  IMPLEMENT(div, 
      M.setRegister(I.RD, M.getRegister(I.RS) / M.getRegister(I.RT));
    );
  
  IMPLEMENT(mod, 
      M.setRegister(I.RV, M.getRegister(I.RS) % M.getRegister(I.RT));
    );

  IMPLEMENT(divu, 
      M.setRegister(I.RD, (uint32_t) M.getRegister(I.RS) / (uint32_t) M.getRegister(I.RT));
    );

  IMPLEMENT(modu, 
      M.setRegister(I.RV, (uint32_t) M.getRegister(I.RS) % (uint32_t) M.getRegister(I.RT));
    );

  IMPLEMENT(ldhu, 
      unsigned short int half = M.getMemHalfAt(M.getRegister(I.RS) + I.Imm);
      M.setRegister(I.RT, (uint32_t) half);
    );

  IMPLEMENT(ldh, 
      short int half = M.getMemHalfAt(M.getRegister(I.RS) + I.Imm);
			M.setRegister(I.RT, (int32_t) half);
    );

  IMPLEMENT(sth, 
      uint16_t half = M.getRegister(I.RT) & 0xFFFF;
      M.setMemByteAt(M.getRegister(I.RS) + I.Imm    , (half)      & 0xFF);
      M.setMemByteAt(M.getRegister(I.RS) + I.Imm + 1, (half >> 8) & 0xFF);
    );
  
  IMPLEMENT(ldi,
      M.setRegister(LDI_REG, I.RT);
      M.setRegister(I.RT, (M.getRegister(I.RT) & 0xFFFFC000) | (I.Imm & 0x3FFF));
    );
  
  IMPLEMENT(ldihi,
      M.setRegister(M.getRegister(LDI_REG), (M.getRegister(M.getRegister(LDI_REG)) & 0x3FFF) | (I.Addrs << 14));
    );
  
  IMPLEMENT(ldw,
      M.setRegister(I.RT, M.getMemValueAt(M.getRegister(I.RS) + I.Imm).asI_);
    );
  
  IMPLEMENT(addi,
      M.setRegister(I.RT, M.getRegister(I.RS) + I.Imm);
    );
  
  IMPLEMENT(and_,
      M.setRegister(I.RD, M.getRegister(I.RS) & M.getRegister(I.RT));
    );
  
  IMPLEMENT(andi,
      M.setRegister(I.RT, M.getRegister(I.RS) & (I.Imm & 0x3FFF));
    );
  
  IMPLEMENT(or_,
      M.setRegister(I.RD, M.getRegister(I.RS) | M.getRegister(I.RT));
    );
  
  IMPLEMENT(nor,
      M.setRegister(I.RD, ~(M.getRegister(I.RS) | M.getRegister(I.RT)));
    );
  
  IMPLEMENT(shr,
      unsigned aux = ((uint32_t) M.getRegister(I.RT)) >> (uint32_t) I.RS; 
      M.setRegister(I.RD, aux);
    );
  
  IMPLEMENT(asr,
      M.setRegister(I.RD, ((int32_t) M.getRegister(I.RT)) >> I.RS);
    );
  
  IMPLEMENT(shl,
      M.setRegister(I.RD, M.getRegister(I.RT) << I.RS);
    );

  IMPLEMENT(shlr,
      M.setRegister(I.RD, M.getRegister(I.RT) << (M.getRegister(I.RS) & 0x1F));
    );
  
  IMPLEMENT(movn,
      if (M.getRegister(I.RT) != 0)
        M.setRegister(I.RD, M.getRegister(I.RS));
    );
  
  IMPLEMENT(movz,
      if (M.getRegister(I.RT) == 0)
        M.setRegister(I.RD, M.getRegister(I.RS));
    );
  
  IMPLEMENT(ldb,
      M.setRegister(I.RT, (int32_t) M.getMemByteAt(M.getRegister(I.RS) + I.Imm));
    );
  
  IMPLEMENT(ldbu,
      M.setRegister(I.RT, (uint32_t) M.getMemByteAt(M.getRegister(I.RS) + I.Imm));
    );

  IMPLEMENT(seh,
      M.setRegister(I.RS, (int32_t) ((int16_t) M.getRegister(I.RT)));
    );

  IMPLEMENT(seb,
      M.setRegister(I.RS, (int32_t) ((int8_t) M.getRegister(I.RT)));
    );
  
  IMPLEMENT(stb,
      M.setMemByteAt(M.getRegister(I.RS) + I.Imm, (unsigned char) M.getRegister(I.RT) & 0xFF);
    );
  
  IMPLEMENT(stw,
      M.setMemValueAt(M.getRegister(I.RS) + I.Imm, M.getRegister(I.RT));
    );
  
  IMPLEMENT(sltiu,
      M.setRegister(I.RT, ((uint32_t) M.getRegister(I.RS)) < ((uint32_t) (I.Imm & 0x3FFF)));
    );
  
  IMPLEMENT(slti,
      M.setRegister(I.RT, (int32_t) M.getRegister(I.RS) < (int32_t) I.Imm);
    );
  
  IMPLEMENT(sltu,
      M.setRegister(I.RD, (uint32_t) M.getRegister(I.RS) < (uint32_t) M.getRegister(I.RT));
    );
  
  IMPLEMENT(slt,
      M.setRegister(I.RD, (int32_t) M.getRegister(I.RS) < (int32_t) M.getRegister(I.RT));
    );
  
  IMPLEMENT(xori,
      M.setRegister(I.RT, M.getRegister(I.RS) ^ (I.Imm & 0x3FFF));
    );

  IMPLEMENT(xor_,
      M.setRegister(I.RD, M.getRegister(I.RS) ^ M.getRegister(I.RT));
    );
  
  IMPLEMENT(ori,
      M.setRegister(I.RT, M.getRegister(I.RS) | (I.Imm & 0x3FFF));
    );

  IMPLEMENT(ijmphi, 
        M.setRegister(IJMP_REG, 0);
        M.setRegister(IJMP_REG, M.getRegister(IJMP_REG) | I.Addrs << 12); 
    );
  
  /**********************  Float Inst  **************************/

   IMPLEMENT(ldc1, 
      M.setRegister(130 + I.RT + 1, M.getMemValueAt(M.getRegister(I.RS) + I.Imm).asI_);
      M.setRegister(130 + I.RT    , M.getMemValueAt(M.getRegister(I.RS) + I.Imm + 4).asI_);
    );

   IMPLEMENT(sdc1, 
      M.setMemValueAt(M.getRegister(I.RS) + I.Imm + 4, M.getRegister(130 + I.RT));
      M.setMemValueAt(M.getRegister(I.RS) + I.Imm    , M.getRegister(130 + I.RT + 1));
    );

   IMPLEMENT(mtlc1, 
       double Temp = M.getDoubleRegister(I.RT);
       uint64_t ToInt;
       memcpy(&ToInt, &Temp, sizeof(uint64_t));
       ToInt = (ToInt & 0xFFFFFFFF00000000ULL) + (((uint64_t) M.getDoubleRegister(I.RS)));
       memcpy(&Temp, &ToInt, sizeof(uint64_t));
       M.setDoubleRegister(I.RT, Temp);
    );

   IMPLEMENT(mthc1, 
       double Temp = M.getDoubleRegister(I.RT);
       uint64_t ToInt;
       memcpy(&ToInt, &Temp, sizeof(uint64_t));
       ToInt = (ToInt & 0xFFFFFFFFULL) + (((uint64_t) M.getDoubleRegister(I.RS)) << 32);
       memcpy(&Temp, &ToInt, sizeof(uint64_t));
       M.setDoubleRegister(I.RT, Temp);
    );

   IMPLEMENT(ceqd, 
       double A = M.getDoubleRegister(I.RS);
       double B = M.getDoubleRegister(I.RT);
       M.setRegister(CC_REG, A == B ? (isnan(A) || isnan(B) ? 0 : 1) : 0);
    );

   IMPLEMENT(ceqs, 
       float A = M.getFloatRegister(I.RS);
       float B = M.getFloatRegister(I.RT);
       M.setRegister(CC_REG, A == B ? (isnan(A) || isnan(B) ? 0 : 1) : 0);
    );

   IMPLEMENT(movd, 
       M.setDoubleRegister(I.RS, M.getDoubleRegister(I.RT));
    );

   IMPLEMENT(lwc1, 
       M.setFloatRegister(I.RT, M.getMemValueAt(M.getRegister(I.RS) + I.Imm).asF_);
    );

   IMPLEMENT(adds, 
       M.setFloatRegister(I.RD, M.getFloatRegister(I.RS) + M.getFloatRegister(I.RT));
    );

   IMPLEMENT(mtc1, 
       M.setFloatRegister(I.RT, M.getRegister(I.RS));
    );

   IMPLEMENT(mfc1, 
       M.setRegister(I.RS, M.getFloatRegister(I.RT));
    );

   IMPLEMENT(truncws, 
       M.setFloatRegister(I.RS, (int32_t) M.getFloatRegister(I.RT));
    );

   IMPLEMENT(cvtsw, 
       float Tmp = (float) (int) M.getFloatRegister(I.RT);
       M.setFloatRegister(I.RS, Tmp);
    );

  /********************** JMPs and BRs **************************/

  IMPLEMENT_BR(jeq,
      if (M.getRegister(I.RS) == M.getRegister(I.RT)) { 
        M.setPC(M.getPC() + (I.Imm << 2) + 4);
        ImplRFT.onBranch(M);
        GOTO_NEXT;
      }
    );
  
  IMPLEMENT_BR(jeqz,
      if (M.getRegister(I.RS) == 0) { 
        M.setPC(M.getPC() + (I.Imm << 2) + 4);
        ImplRFT.onBranch(M);
        GOTO_NEXT;
      }
    );
  
  IMPLEMENT_BR(jgtz,
      if (!(M.getRegister(I.RT) & 0x80000000) && (M.getRegister(I.RT) != 0)) { 
        M.setPC(M.getPC() + (I.Imm << 2) + 4);
        ImplRFT.onBranch(M);
        GOTO_NEXT;
      }
    );
  
  IMPLEMENT_BR(jgez,
      if (!(M.getRegister(I.RT) & 0x80000000)) { 
        M.setPC(M.getPC() + (I.Imm << 2) + 4);
        ImplRFT.onBranch(M);
        GOTO_NEXT;
      }
    );
  
  IMPLEMENT_BR(jlez,
      if ((M.getRegister(I.RT) == 0) || (M.getRegister(I.RT) & 0x80000000)) { 
        M.setPC(M.getPC() + (I.Imm << 2) + 4);
        ImplRFT.onBranch(M);
        GOTO_NEXT;
      }
    );
  
  IMPLEMENT_BR(jltz,
      if (M.getRegister(I.RT) & 0x80000000) { 
        M.setPC(M.getPC() + (I.Imm << 2) + 4);
        ImplRFT.onBranch(M);
        GOTO_NEXT;
      }
    );
  
  IMPLEMENT_BR(jne,
      if (M.getRegister(I.RS) != M.getRegister(I.RT)) { 
        M.setPC(M.getPC() + (I.Imm << 2) + 4);
        ImplRFT.onBranch(M);
        GOTO_NEXT;
      }
    );
  
  IMPLEMENT_BR(jnez,
      if (M.getRegister(I.RS) != 0) { 
        M.setPC(M.getPC() + (I.Imm << 2) + 4);
        ImplRFT.onBranch(M);
        GOTO_NEXT;
      }
    );

   IMPLEMENT_BR(bc1f, 
       if (M.getRegister(CC_REG) == 0) {
         M.setPC(M.getPC() + (I.Imm << 2));
         ImplRFT.onBranch(M);
         GOTO_NEXT;
       }
    );

   IMPLEMENT_BR(bc1t, 
       if (M.getRegister(CC_REG) == 1) {
         M.setPC(M.getPC() + (I.Imm << 2));
         ImplRFT.onBranch(M);
         GOTO_NEXT;
       }
    );

  IMPLEMENT_JMP(call,
      M.setRegister(31, M.getPC()+4);
      M.setPC((M.getPC() & 0xF0000000) | (I.Addrs << 2));
    );

  IMPLEMENT_JMP(callr,
      M.setRegister(31, M.getPC()+4);
      M.setPC(M.getRegister(I.RT));
    );

  IMPLEMENT_JMP(jumpr,
      M.setPC(M.getRegister(I.RT));
    );
  
  IMPLEMENT_JMP(jump,
      M.setPC((M.getPC() & 0xF0000000) | (I.Addrs << 2));
    );

  IMPLEMENT_JMP(ijmp,
      M.setRegister(IJMP_REG, M.getRegister(IJMP_REG) & 0xFFFFF000); 
      M.setRegister(IJMP_REG, M.getRegister(IJMP_REG) | (I.Imm & 0xFFF)); 
      uint32_t Target = M.getMemValueAt(M.getRegister(IJMP_REG) + M.getRegister(I.RT)).asI_;
      M.setPC(Target);
    );
  
	IMPLEMENT(syscall,
    	if (SyscallM.processSyscall(M))
      	return;
		);

  // --------------------------------------------------------------------------------------------------- //
}

void ITDInterpreter::execute(Machine& M, uint32_t StartAddrs, uint32_t EndAddrs) {
  if (DispatchValues.size() == 0 || !isAddrsContainedIn(StartAddrs, EndAddrs)) { 
    DispatchValues.reserve((EndAddrs - StartAddrs)/4);
    DecodedInsts.reserve((EndAddrs - StartAddrs)/4);
  }

  LastStartAddrs = StartAddrs;
  LastEndAddrs   = EndAddrs;

  dispatch(M, StartAddrs, EndAddrs);
}
