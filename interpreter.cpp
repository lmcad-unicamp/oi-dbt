#include <interpreter.hpp>

#include <cmath>
#include <iostream>

using namespace dbt;
using namespace dbt::OIDecoder;

#ifdef COLOR
#define COLOR_CYAN "\033[1;35m"
#define COLOR_NONE "\033[0m"
#else
#define COLOR_CYAN ""
#define COLOR_NONE ""
#endif

//#define PRINTINST

uint64_t instacc = 0;
#ifdef PRINTINST
#include <OIPrinter.hpp>
//#define DEBUG_PRINT(Addr, Inst) std::cerr << OIPrinter::getString(Inst) << "\n";
//#define DEBUG_PRINT(Addr, Inst) std::cerr << /*std::dec << (++instacc) <<" -- "<<*/ std::hex << Addr << "\t" << OIPrinter::getString(Inst) << std::dec << "\n";
//#define DEBUG_PRINT(Addr, Inst) std::cerr << "\n" << COLOR_CYAN <<  std::dec << (++instacc) <<" -- "<< std::hex << Addr << "\t" << OIPrinter::getString(Inst) << std::dec << COLOR_NONE << "\t";
#define DEBUG_PRINT(Addr, Inst) std::cerr << "\n" << COLOR_CYAN <<  std::dec << (++instacc) <<" -- "<< std::hex << Addr << "\t" << OIPrinter::getString(Inst) << std::dec << COLOR_NONE << "\t";
#else
#define DEBUG_PRINT(Addr, Inst)
#endif

#define SET_DISPACH(Addrs, Label, Offset)\
  case Label:\
    setDispatchValue(Addrs, static_cast<int*>(Offset));\
    break;

#define GOTO_NEXT\
    DEBUG_PRINT(M.getPC(), getDecodedInst(M.getPC()))\
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
    /*M.dumpRegisters();*/\
    I = getDecodedInst(M.getPC());\
    {\
      Code\
      ImplRFT.onBranch(M);\
    }\
    GOTO_NEXT

#define IMPLEMENT_BR(Label, Code)\
  Label:\
    /*M.dumpRegisters();*/\
    I = getDecodedInst(M.getPC());\
    {\
      Code\
    }\
    M.incPC();\
    GOTO_NEXT

//#define rotate_right(x, n) (((x) >> (n)) | ((x) << ((sizeof(x) * 8) - (n))))
static inline uint32_t rotate_right(uint32_t input, uint32_t shiftamount) {
  return (((uint32_t)input) >> shiftamount) |
         (((uint32_t)input) << (32 - shiftamount));
}

typedef union DWordBit {
  double asF;
  uint64_t asI;
} DWordBit;

typedef union WordBit {
  float asF;
  int32_t asI;
} WordBit;

bool isnan(double x) { return x != x; }
bool isnan(float x)  { return x != x; }

bool ITDInterpreter::isAddrsContainedIn(uint32_t StartAddrs, uint32_t EndAddrs) {
  return !(StartAddrs < LastStartAddrs || EndAddrs > LastEndAddrs);
}

inline void* ITDInterpreter::getDispatchValue(uint32_t Addrs) {
  return (void*)DispatchValues[(Addrs-LastStartAddrs)/4];
}

void ITDInterpreter::setDispatchValue(uint32_t Addrs, int* Target) {
  DispatchValues[(Addrs-LastStartAddrs)/4] = Target;
}

inline OIInst ITDInterpreter::getDecodedInst(uint32_t Addrs) {
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
      SET_DISPACH(Addrs, Absd,    &&absd);
      SET_DISPACH(Addrs, Abss,    &&abss);
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
      SET_DISPACH(Addrs, Asrr,     &&asrr);
      SET_DISPACH(Addrs, Shl,     &&shl);
      SET_DISPACH(Addrs, Shlr,    &&shlr);
      SET_DISPACH(Addrs, Shrr,    &&shrr);
      SET_DISPACH(Addrs, Movn,    &&movn);
      SET_DISPACH(Addrs, Movz,    &&movz);
      SET_DISPACH(Addrs, Ror,     &&ror);
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
      SET_DISPACH(Addrs, Sdxc1,   &&sdxc1);
      SET_DISPACH(Addrs, Ldxc1,   &&ldxc1);
      SET_DISPACH(Addrs, Mtlc1,   &&mtlc1);
      SET_DISPACH(Addrs, Mthc1,   &&mthc1);
      SET_DISPACH(Addrs, Ceqs,    &&ceqs);
      SET_DISPACH(Addrs, Ceqd,    &&ceqd);
      SET_DISPACH(Addrs, Bc1f,    &&bc1f);
      SET_DISPACH(Addrs, Bc1t,    &&bc1t);
      SET_DISPACH(Addrs, Movd,    &&movd);
      SET_DISPACH(Addrs, Movf,    &&movf);
      SET_DISPACH(Addrs, Movt,    &&movt);
      SET_DISPACH(Addrs, Movts,   &&movts);
      SET_DISPACH(Addrs, Movs,    &&movs);
      SET_DISPACH(Addrs, Movzd,   &&movzd);
      SET_DISPACH(Addrs, Movzs,   &&movzs);
      SET_DISPACH(Addrs, Movnd,   &&movnd);
      SET_DISPACH(Addrs, Movns,   &&movns);
      SET_DISPACH(Addrs, Movtd,   &&movtd);
      SET_DISPACH(Addrs, Movfd,   &&movfd);
      SET_DISPACH(Addrs, Movfs,   &&movfs);
      SET_DISPACH(Addrs, Lwc1,    &&lwc1);
      SET_DISPACH(Addrs, Adds,    &&adds);
      SET_DISPACH(Addrs, Addd,    &&addd);
      SET_DISPACH(Addrs, Maddd,   &&maddd);
      SET_DISPACH(Addrs, Mtc1,    &&mtc1);
      SET_DISPACH(Addrs, Mfc1,    &&mfc1);
      SET_DISPACH(Addrs, Truncws, &&truncws);
      SET_DISPACH(Addrs, Truncwd, &&truncwd);
      SET_DISPACH(Addrs, Cvtsw,   &&cvtsw);
      SET_DISPACH(Addrs, Cvtdw,   &&cvtdw);
      SET_DISPACH(Addrs, Cvtds,   &&cvtds);
      SET_DISPACH(Addrs, Cvtsd,   &&cvtsd);
      SET_DISPACH(Addrs, Lwxc1,   &&lwxc1);
      SET_DISPACH(Addrs, Swc1,    &&swc1);
      SET_DISPACH(Addrs, Swxc1,   &&swxc1);
      SET_DISPACH(Addrs, Muls,    &&muls);
      SET_DISPACH(Addrs, Muld,    &&muld);
      SET_DISPACH(Addrs, Coltd,   &&coltd);
      SET_DISPACH(Addrs, Colts,   &&colts);
      SET_DISPACH(Addrs, Coled,   &&coled);
      SET_DISPACH(Addrs, Coles,   &&coles);
      SET_DISPACH(Addrs, Culed,   &&culed);
      SET_DISPACH(Addrs, Cults,   &&cults);
      SET_DISPACH(Addrs, Cultd,   &&cultd);
      SET_DISPACH(Addrs, Cules,   &&cules);
      SET_DISPACH(Addrs, Cuns,    &&cuns);
      SET_DISPACH(Addrs, Cueqd,    &&cueqd);
      SET_DISPACH(Addrs, Cund,    &&cund);
      SET_DISPACH(Addrs, Negd,    &&negd);
      SET_DISPACH(Addrs, Negs,    &&negs);
      SET_DISPACH(Addrs, Divs,    &&divs);
      SET_DISPACH(Addrs, Divd,    &&divd);
      SET_DISPACH(Addrs, Subd,    &&subd);
      SET_DISPACH(Addrs, Subs,    &&subs);
      SET_DISPACH(Addrs, Mflc1,   &&mflc1);
      SET_DISPACH(Addrs, Mfhc1,   &&mfhc1);
      SET_DISPACH(Addrs, Msubs,   &&msubs);
      SET_DISPACH(Addrs, Msubd,   &&msubd);
      SET_DISPACH(Addrs, Madds,   &&madds);
      SET_DISPACH(Addrs, Sqrts,   &&sqrts);
      SET_DISPACH(Addrs, Sqrtd,   &&sqrtd);
      SET_DISPACH(Addrs, Ext,     &&ext);
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

      M.setRegister(I.RD, M.getRegister(I.RS) + M.getRegister(I.RT));
    );

  IMPLEMENT(sub,
      M.setRegister(I.RD, M.getRegister(I.RS) - M.getRegister(I.RT));
    );

  IMPLEMENT(mul,
      int64_t Result;
      int32_t Half;
      Result =  (int32_t) M.getRegister(I.RS);
      Result *= (int32_t) M.getRegister(I.RT);

      Half = (Result & 0xFFFFFFFF);
      if (I.RD != 0)
        M.setRegister(I.RD, Half);

      Half = ((Result >> 32) & 0xFFFFFFFF);
      if (I.RV != 0)
        M.setRegister(I.RV, Half);
    );

  IMPLEMENT(mulu,
      uint64_t Result;
      int32_t Half;
      Result =  (uint32_t) M.getRegister(I.RS);
      Result *= (uint32_t) M.getRegister(I.RT);

      Half = (Result & 0xFFFFFFFF);
      if (I.RD != 0)
        M.setRegister(I.RD, Half);

      Half = ((Result >> 32) & 0xFFFFFFFF);
      if (I.RV != 0)
        M.setRegister(I.RV, Half);
    );

  IMPLEMENT(ext,
     uint32_t Lsb = I.RS;
     uint32_t Size = I.RT+1;
     M.setRegister(I.RV, (((unsigned)M.getRegister(I.RD)) << (32 - Size - Lsb)) >> (32 - Size));
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

  IMPLEMENT(asrr,
      M.setRegister(I.RD, ((int32_t) M.getRegister(I.RT)) >> (M.getRegister(I.RS) & 0x1F));
    );

  IMPLEMENT(shl,
      M.setRegister(I.RD, M.getRegister(I.RT) << I.RS);
    );

  IMPLEMENT(shlr,
      M.setRegister(I.RD, M.getRegister(I.RT) << (M.getRegister(I.RS) & 0x1F));
    );

  IMPLEMENT(shrr,
      M.setRegister(I.RD, (uint32_t) M.getRegister(I.RT) >> (M.getRegister(I.RS) & 0x1F));
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
      if (((uint32_t) M.getRegister(I.RS)) < ((uint32_t) (I.Imm & 0x3FFF)))
        M.setRegister(I.RT, 1);
      else
        M.setRegister(I.RT, 0);
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

  IMPLEMENT(ror,
      M.setRegister(I.RD, rotate_right(M.getRegister(I.RT), I.RS));
    );

  IMPLEMENT(ijmphi,
      M.setRegister(IJMP_REG, 0 | (I.Addrs << 12));
    );

  /**********************  Float Inst  **************************/

   IMPLEMENT(absd,
       M.setDoubleRegister(I.RS, fabs(M.getDoubleRegister(I.RT)));
    );

   IMPLEMENT(abss,
       M.setFloatRegister(I.RS, fabs(M.getFloatRegister(I.RT)));
    );

   IMPLEMENT(ldc1,
			int32_t* R = M.getRegisterPtr();
			char* M1 = M.getByteMemoryPtr();	
			uint32_t Offset = M.getDataMemOffset();
			
			*(((int64_t*) R) + 65 + I.RT) = *((int64_t*) ((M1 + I.Imm + (*(R + I.RS))) - Offset));
    );

   IMPLEMENT(lwc1,
       M.setFloatRegister(I.RT, M.getMemValueAt(M.getRegister(I.RS) + I.Imm).asF_);
    );

   IMPLEMENT(lwxc1,
       M.setFloatRegister(I.RD, M.getMemValueAt(M.getRegister(I.RT) + M.getRegister(I.RS)).asF_);
    );

   IMPLEMENT(ldxc1,
			int32_t* R = M.getRegisterPtr();
			char* M1 = M.getByteMemoryPtr();	
			uint32_t Offset = M.getDataMemOffset();
			
			*(((int64_t*) R) + 65 + I.RD) = *((int64_t*) ((M1 + (*(R + I.RT)) + (*(R + I.RS))) - Offset));
    );

   IMPLEMENT(sdxc1,
      M.setMemValueAt(M.getRegister(I.RT) + M.getRegister(I.RS) +0, M.getRegister(130 + I.RD*2 + 0));
      M.setMemValueAt(M.getRegister(I.RT) + M.getRegister(I.RS) +4 , M.getRegister(130 + I.RD*2 + 1));
    );

   IMPLEMENT(sdc1,
      M.setMemValueAt(M.getRegister(I.RS) + I.Imm +0, M.getRegister(130 + I.RT*2+0));
      M.setMemValueAt(M.getRegister(I.RS) + I.Imm +4, M.getRegister(130 + I.RT*2+1));
    );

   IMPLEMENT(swc1,
      M.setMemValueAt(M.getRegister(I.RS) + I.Imm, M.getRegister(66 + I.RT));
    );

   IMPLEMENT(swxc1,
      M.setMemValueAt(M.getRegister(I.RT) + M.getRegister(I.RS), M.getRegister(66 + I.RD));
    );

   IMPLEMENT(mtlc1,
       DWordBit DW;
       DW.asF = M.getDoubleRegister(I.RT);
       DW.asI = (DW.asI & 0xFFFFFFFF00000000ULL) + (((uint64_t) M.getRegister(I.RS)));
       M.setDoubleRegister(I.RT, DW.asF);
    );

   IMPLEMENT(mthc1,
       DWordBit DW;
       DW.asF = M.getDoubleRegister(I.RT);
       DW.asI = (DW.asI & 0xFFFFFFFFULL) + (((uint64_t) M.getRegister(I.RS)) << 32);
       M.setDoubleRegister(I.RT, DW.asF);
    );

   IMPLEMENT(mflc1,
       DWordBit DW;
       DW.asF = M.getDoubleRegister(I.RT);
       M.setRegister(I.RS, (uint32_t)(DW.asI & 0xFFFFFFFF));
   );

   IMPLEMENT(mfhc1,
       DWordBit DW;
       DW.asF = M.getDoubleRegister(I.RT);
       M.setRegister(I.RS, (uint32_t)(DW.asI >> 32));
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

   IMPLEMENT(negd,
       M.setDoubleRegister(I.RS, -M.getDoubleRegister(I.RT));
    );

   IMPLEMENT(negs,
       M.setFloatRegister(I.RS, -M.getFloatRegister(I.RT));
    );

   IMPLEMENT(movd,
       M.setDoubleRegister(I.RS, M.getDoubleRegister(I.RT));
    );

   IMPLEMENT(movf,
      if (M.getRegister(CC_REG) == 0)
        M.setRegister(I.RS, M.getRegister(I.RT));
    );

   IMPLEMENT(movt,
      if (M.getRegister(CC_REG) != 0)
        M.setRegister(I.RS, M.getRegister(I.RT));
    );

   IMPLEMENT(movts,
      if (M.getRegister(CC_REG) != 0)
        M.setFloatRegister(I.RS, M.getFloatRegister(I.RT));
    );
   IMPLEMENT(movs,
       M.setFloatRegister(I.RS, M.getFloatRegister(I.RT));
    );

   IMPLEMENT(movzd,
       if (M.getRegister(I.RT) == 0)
        M.setDoubleRegister(I.RD, M.getDoubleRegister(I.RS));
    );

   IMPLEMENT(movzs,
       if (M.getRegister(I.RT) == 0)
        M.setFloatRegister(I.RD, M.getFloatRegister(I.RS));
    );

   IMPLEMENT(movnd,
       if (M.getRegister(I.RT) != 0)
        M.setDoubleRegister(I.RD, M.getDoubleRegister(I.RS));
    );

   IMPLEMENT(movns,
       if (M.getRegister(I.RT) != 0)
        M.setFloatRegister(I.RD, M.getFloatRegister(I.RS));
    );

   IMPLEMENT(movfd,
       if (M.getRegister(CC_REG) == 0)
        M.setDoubleRegister(I.RS, M.getDoubleRegister(I.RT));
    );

   IMPLEMENT(movfs,
       if (M.getRegister(CC_REG) == 0)
        M.setFloatRegister(I.RS, M.getFloatRegister(I.RT));
    );

   IMPLEMENT(movtd,
       if (M.getRegister(CC_REG) != 0)
        M.setDoubleRegister(I.RS, M.getDoubleRegister(I.RT));
    );

   IMPLEMENT(adds,
       M.setFloatRegister(I.RD, M.getFloatRegister(I.RS) + M.getFloatRegister(I.RT));
    );

   IMPLEMENT(subd,
       M.setDoubleRegister(I.RD, M.getDoubleRegister(I.RS) - M.getDoubleRegister(I.RT));
    );

   IMPLEMENT(subs,
       M.setFloatRegister(I.RD, M.getFloatRegister(I.RS) - M.getFloatRegister(I.RT));
    );

   IMPLEMENT(muls,
       M.setFloatRegister(I.RD, M.getFloatRegister(I.RS) * M.getFloatRegister(I.RT));
    );

   IMPLEMENT(muld,
       M.setDoubleRegister(I.RD, M.getDoubleRegister(I.RS) * M.getDoubleRegister(I.RT));
    );

   IMPLEMENT(divd,
       M.setDoubleRegister(I.RD, M.getDoubleRegister(I.RS) / M.getDoubleRegister(I.RT));
    );

   IMPLEMENT(divs,
       M.setFloatRegister(I.RD, M.getFloatRegister(I.RS) / M.getFloatRegister(I.RT));
    );

   IMPLEMENT(addd,
       M.setDoubleRegister(I.RD, M.getDoubleRegister(I.RS) + M.getDoubleRegister(I.RT));
   );

   IMPLEMENT(maddd,
       M.setDoubleRegister(I.RD, M.getDoubleRegister(I.RS) * M.getDoubleRegister(I.RT) + M.getDoubleRegister(I.RV));
   );

   IMPLEMENT(msubs,
       M.setFloatRegister(I.RD, M.getFloatRegister(I.RS) * M.getFloatRegister(I.RT) - M.getFloatRegister(I.RV));
   );

   IMPLEMENT(msubd,
       M.setDoubleRegister(I.RD, M.getDoubleRegister(I.RS) * M.getDoubleRegister(I.RT) - M.getDoubleRegister(I.RV));
   );

   IMPLEMENT(madds,
       M.setFloatRegister(I.RD, M.getFloatRegister(I.RS) * M.getFloatRegister(I.RT) + M.getFloatRegister(I.RV));
   );

   IMPLEMENT(mtc1,
       WordBit Tmp;
       Tmp.asI = M.getRegister(I.RS);
       M.setFloatRegister(I.RT, Tmp.asF);
    );

   IMPLEMENT(mfc1,
       WordBit Tmp;
       Tmp.asF = M.getFloatRegister(I.RT);
       M.setRegister(I.RS, Tmp.asI);
    );

   IMPLEMENT(truncws,
       WordBit Tmp;
       Tmp.asI = (int32_t) M.getFloatRegister(I.RT);
       M.setFloatRegister(I.RS, Tmp.asF);
    );

   IMPLEMENT(truncwd,
       WordBit Tmp;
       Tmp.asI = (int32_t) M.getDoubleRegister(I.RT);
       M.setFloatRegister(I.RS, Tmp.asF);
    );

   IMPLEMENT(cvtsw,
       WordBit Tmp;
       Tmp.asF = M.getFloatRegister(I.RT);
       M.setFloatRegister(I.RS, (float) (int) Tmp.asI);
    );

   IMPLEMENT(cvtdw,
       WordBit Tmp;
       Tmp.asF = M.getFloatRegister(I.RT);
       M.setDoubleRegister(I.RS, (double) (int) Tmp.asI);
    );

   IMPLEMENT(cvtds,
       M.setDoubleRegister(I.RS, (double) M.getFloatRegister(I.RT));
    );

   IMPLEMENT(cvtsd,
       M.setFloatRegister(I.RS, (float) M.getDoubleRegister(I.RT));
    );

   IMPLEMENT(coltd,
       double A = M.getDoubleRegister(I.RS);
       double B = M.getDoubleRegister(I.RT);
       M.setRegister(CC_REG, A < B ? (isnan(A) || isnan(B) ? 0 : 1) : 0);
    );

   IMPLEMENT(colts,
       double A = M.getFloatRegister(I.RS);
       double B = M.getFloatRegister(I.RT);
       M.setRegister(CC_REG, A < B ? (isnan(A) || isnan(B) ? 0 : 1) : 0);
    );

   IMPLEMENT(coled,
       double A = M.getDoubleRegister(I.RS);
       double B = M.getDoubleRegister(I.RT);
       M.setRegister(CC_REG, A <= B ? (isnan(A) || isnan(B) ? 0 : 1) : 0);
    );

   IMPLEMENT(coles,
       float A = M.getFloatRegister(I.RS);
       float B = M.getFloatRegister(I.RT);
       M.setRegister(CC_REG, A <= B ? (isnan(A) || isnan(B) ? 0 : 1) : 0);
    );

   IMPLEMENT(culed,
       M.setRegister(CC_REG, M.getDoubleRegister(I.RS) <= M.getDoubleRegister(I.RT) ? 1 : 0);
    );

   IMPLEMENT(cules,
       M.setRegister(CC_REG, M.getFloatRegister(I.RS) <= M.getFloatRegister(I.RT) ? 1 : 0);
    );

   IMPLEMENT(cults,
       M.setRegister(CC_REG, M.getFloatRegister(I.RS) < M.getFloatRegister(I.RT) ? 1 : 0);
    );

   IMPLEMENT(cultd,
       M.setRegister(CC_REG, M.getDoubleRegister(I.RS) < M.getDoubleRegister(I.RT) ? 1 : 0);
    );

   IMPLEMENT(cund,
       M.setRegister(CC_REG, (isnan(M.getDoubleRegister(I.RS)) || isnan(M.getDoubleRegister(I.RT))) ? 1 : 0);
    );

   IMPLEMENT(cuns,
       M.setRegister(CC_REG, (isnan(M.getFloatRegister(I.RS)) || isnan(M.getFloatRegister(I.RT))) ? 1 : 0);
    );


   IMPLEMENT(cueqd,
       M.setRegister(CC_REG, (M.getFloatRegister(I.RS) == M.getFloatRegister(I.RT)) ? 1 : 0);
    );

   IMPLEMENT(sqrtd,
       M.setDoubleRegister(I.RS, sqrt(M.getDoubleRegister(I.RT)));
    );

   IMPLEMENT(sqrts,
       M.setFloatRegister(I.RS, sqrt(M.getFloatRegister(I.RT)));
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
         M.setPC(M.getPC() + (I.Imm << 2) + 4);
         ImplRFT.onBranch(M);
         GOTO_NEXT;
       }
    );

   IMPLEMENT_BR(bc1t,
       if (M.getRegister(CC_REG) == 1) {
         M.setPC(M.getPC() + (I.Imm << 2) + 4);
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
    DecodedInsts  .reserve((EndAddrs - StartAddrs)/4);
  }

  LastStartAddrs = StartAddrs;
  LastEndAddrs   = EndAddrs;

  dispatch(M, StartAddrs, EndAddrs);
}
