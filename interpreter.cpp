#include <OIDecoder.hpp>
#include <interpreter.hpp>
#include <timer.hpp>

#include <cmath>
#include <ctime>
#include <iostream>

using namespace dbt;
using namespace dbt::OIDecoder;
unsigned total = 0;

#include <OIPrinter.hpp>
#define DEBUG_PRINT(Addr, Inst) std::cout << std::hex << Addr << "\t" << OIPrinter::getString(Inst) << std::dec << "\n";

#define SET_DISPACH(Addrs, Label, Offset)\
  case Label:\
    setDispatchValue(Addrs, static_cast<int*>(Offset));\
    break;

#define IMPLEMENT(Label, Code)\
  Label:\
    I = getDecodedInst(M.getPC());\
    {\
       Code\
    }\
    M.incPC();\
    goto next;

#define IMPLEMENT_JMP(Label, Code)\
  Label:\
    I = getDecodedInst(M.getPC());\
    {\
      Code\
    }\
    ImplRFT.onBranch(M);\
    goto next;

#define IMPLEMENT_BR(Label, Code)\
  Label:\
    I = getDecodedInst(M.getPC());\
    M.incPC();\
    {\
      Code\
    }\
    goto next;


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
      SET_DISPACH(Addrs, Nop,     &&nop);
      case Null:
        exit(1);
    }
    setDecodedInst(Addrs, I);
  }

  // ---------------------------------------- Trampoline Zone ------------------------------------------ //

  OIInst I;
  constexpr int32_t ldiReg = 64;
  goto next;

  IMPLEMENT(nop, );
  
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
      M.setRegister(ldiReg, I.RT);
      M.setRegister(I.RT, (M.getRegister(I.RT) & 0xFFFFC000) | (I.Imm & 0x3FFF));
    );
  
  IMPLEMENT(ldihi,
      M.setRegister(M.getRegister(ldiReg), (M.getRegister(M.getRegister(ldiReg)) & 0x3FFF) | (I.Addrs << 14));
    );
  
  IMPLEMENT(ldw,
			//std::cout << M.getRegister(I.RS) + I.Imm << " | "<< (M.getMemValueAt(M.getRegister(I.RS) + I.Imm).asI_) << "\n";
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
  
  IMPLEMENT_JMP(call,
      M.setRegister(31, M.getPC()+4);
      M.setPC((M.getPC() & 0xF0000000) | (I.Addrs << 2));
    );

  IMPLEMENT_JMP(callr,
      M.setRegister(31, M.getPC()+4);
      M.setPC(M.getRegister(I.RT));
    );
  
  IMPLEMENT(ldb,
      M.setRegister(I.RT, (int32_t) M.getMemByteAt(M.getRegister(I.RS) + I.Imm));
    );
  
  IMPLEMENT(ldbu,
      M.setRegister(I.RT, (uint32_t) M.getMemByteAt(M.getRegister(I.RS) + I.Imm));
    );

  IMPLEMENT(seh,
      M.setRegister(I.RS, (M.getRegister(I.RT) >> 16) << 16);
    );
  
  IMPLEMENT(stb,
      M.setMemByteAt(M.getRegister(I.RS) + I.Imm, (unsigned char) M.getRegister(I.RT) & 0xFF);
    );
  
  IMPLEMENT_JMP(jumpr,
      M.setPC(M.getRegister(I.RT));
    );
  
  IMPLEMENT(stw,
      M.setMemValueAt(M.getRegister(I.RS) + I.Imm, M.getRegister(I.RT));
			//std::cout << M.getRegister(I.RS) + I.Imm << " <- " << M.getRegister(I.RT) << "\n";
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
  
  IMPLEMENT_BR(jeq,
      if (M.getRegister(I.RS) == M.getRegister(I.RT)) { 
        M.setPC(M.getPC() + (I.Imm << 2));
        ImplRFT.onBranch(M);
      }
    );
  
  IMPLEMENT_BR(jeqz,
      if (M.getRegister(I.RS) == 0) { 
        M.setPC(M.getPC() + (I.Imm << 2));
        ImplRFT.onBranch(M);
      }
    );
  
  IMPLEMENT_BR(jgtz,
      if (!(M.getRegister(I.RT) & 0x80000000) && (M.getRegister(I.RT) != 0)) { 
        M.setPC(M.getPC() + (I.Imm << 2));
        ImplRFT.onBranch(M);
      }
    );
  
  IMPLEMENT_BR(jgez,
      if (!(M.getRegister(I.RT) & 0x80000000)) { 
        M.setPC(M.getPC() + (I.Imm << 2));
        ImplRFT.onBranch(M);
      }
    );
  
  IMPLEMENT_BR(jlez,
      if ((M.getRegister(I.RT) == 0) || (M.getRegister(I.RT) & 0x80000000)) { 
        M.setPC(M.getPC() + (I.Imm << 2));
        ImplRFT.onBranch(M);
      }
    );
  
  IMPLEMENT_BR(jltz,
      if (M.getRegister(I.RT) & 0x80000000) { 
        M.setPC(M.getPC() + (I.Imm << 2));
        ImplRFT.onBranch(M);
      }
    );
  
  IMPLEMENT_BR(jne,
      if (M.getRegister(I.RS) != M.getRegister(I.RT)) { 
        M.setPC(M.getPC() + (I.Imm << 2));
        ImplRFT.onBranch(M);
      }
    );
  
  IMPLEMENT_BR(jnez,
      if (M.getRegister(I.RS) != 0) { 
        M.setPC(M.getPC() + (I.Imm << 2));
        ImplRFT.onBranch(M);
      }
    );
  
  IMPLEMENT_JMP(jump,
      M.setPC((M.getPC() & 0xF0000000) | (I.Addrs << 2));
    );
  
	IMPLEMENT(syscall,
    	if (SyscallM.processSyscall(M))
      	return;
		);

  next:
    //DEBUG_PRINT(M.getPC(), getDecodedInst(M.getPC()));
    goto *getDispatchValue(M.getPC());

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
