#ifndef OIDECODER_HPP
#define OIDECODER_HPP

#include <memory>
#include <iostream>

#define LDI_REG   64
#define IJMP_REG  65
#define CC_REG    257

//#define PRINTREG

namespace dbt {
  namespace OIDecoder {
    union Word {
      char asC_[4];
      uint32_t asI_;
    };

    enum OIInstType
    { Add  , And , Andi, Or  , Ldi , Ldihi, Ldw , Addi, Call , Jumpr, Stw , Sltiu  , Slti   , Jeq   , Jne  , Jump , Mul  , 
      Shr  , Shl , Jeqz, Sub , Slt , Div  , Mod , Ori , Jgtz , Jlez , Jnez, Ldbu   , Stb    , Sltu  , Asr  , Jltz , Movn ,
      Nor  , Ldh , Ldb , Sth , Ldhu, Jgez , Nop , Seh , Callr, Shlr , Xor , Seb    , Ijmphi , Ijmp  , Divu , Modu , Ldc1 , 
      Mthc1, Ceqd, Ceqs, Bc1f, Bc1t, Movd , Lwc1, Adds, Addd , Mtc1 , Mfc1, Truncws, Truncwd, Cvtsw , Cvtdw, Cvtds, Cvtsd, 
      Mulu , Movz, Xori, Sdc1, Swc1, Maddd, Movs, Muls, Coltd, Swxc1, Negd, Lwxc1  , Syscall, Mtlc1 , Divs , Subs , Mflc1, 
      Mfhc1, Divd, Subd, Negs, Ext , Madds, Shrr, Movf, Movt , Ldxc1, Muld, Sdxc1  , Msubs  , Coled , Culed, Msubd, Movzd,
      Movfd, Asrr, Absd, Abss, Cund, Movnd, Ror, Movzs, Movfs, Colts, Movns, Coles , Sqrtd  , Sqrts , Cults, Cules, Cultd,
      Movtd, Movts,Cuns, Cueqs, Cueqd, Null 
    };

    enum EncodingType {
      PL0, PL6, PL26ij, PL26j, PL26c, PL26i, PL12, PL18, PL16, PL24, PL18i, PL20, PL20i
    };

    typedef struct OIInst {
      OIInstType Type;
      uint8_t RS, RT, RD, RV;
      int16_t Imm;
      uint32_t Addrs;
    } OIInst;

    EncodingType getEncodingType(OIInstType);
    void fillFields(OIInst&, EncodingType, Word); 
    OIInst decode(uint32_t);
    bool isControlFlowInst(OIInst);
    bool isIndirectBranch(OIInst);
    std::array<uint32_t, 2> getPossibleTargets(uint32_t, OIInst);
  }
}

#endif
