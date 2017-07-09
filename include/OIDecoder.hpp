#ifndef OIDECODER_HPP
#define OIDECODER_HPP

#include <memory>
#include <iostream>

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
      Movtd, Null };

    enum EncodingType {
      PL0, PL6, PL26ij, PL26j, PL26c, PL26i, PL12, PL18, PL16, PL24, PL18i, PL20, PL20i
    };

    typedef struct OIInst {
      OIInstType Type;
      uint8_t RS, RT, RD, RV;
      int16_t Imm;
      uint32_t Addrs;
    } OIInst;


    constexpr uint8_t getCount(Word W) {
      return (W.asI_) & 0xFF;
    }

    constexpr uint8_t getIndex(Word W) {
      return (W.asI_ >> 8) & 0x3F;
    }

    constexpr uint16_t getPL12(Word W) {
      return (W.asI_ >> 14) & 0xFFF;
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

    constexpr uint8_t getRV(Word W) {
      return (W.asI_ >> 18) & 0x3F;
    }

    static int16_t getImm(Word W) {
      uint16_t x = (W.asI_ >> 6) & 0x3FFF;
      return (x & 0x2000) ? (x | 0xc000) : x;
    }

    static int16_t getHalfword(Word W) {
      uint16_t x = (W.asI_) & 0x3FFF;
      return (x & 0x2000) ? (x | 0xc000) : x;
    }

    static int16_t getImm1(Word W) {
      uint16_t x = (W.asI_ >> 12) & 0x3FFF;
      return (x & 0x2000) ? (x | 0xc000) : x;
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

    static EncodingType getEncodingType(OIInstType InstType) {
      switch(InstType) {
        case OIInstType::Andi:
        case OIInstType::Stb:
        case OIInstType::Ldbu:
        case OIInstType::Ldb:
        case OIInstType::Ldw:
        case OIInstType::Addi:
        case OIInstType::Stw:
        case OIInstType::Xori:
        case OIInstType::Sltiu:
        case OIInstType::Slti:
        case OIInstType::Jeqz:
        case OIInstType::Jeq:
        case OIInstType::Jnez:
        case OIInstType::Jne:
        case OIInstType::Ori:
        case OIInstType::Sth:
        case OIInstType::Ldh:
        case OIInstType::Ldhu:
        case OIInstType::Sdc1:
        case OIInstType::Ldc1:
        case OIInstType::Lwc1:
        case OIInstType::Swc1:
          return EncodingType::PL26i;
        case OIInstType::Mulu:
        case OIInstType::Mul:
        case OIInstType::Div:
        case OIInstType::Mod:
        case OIInstType::Divu:
        case OIInstType::Modu:
        case OIInstType::Maddd:
        case OIInstType::Madds:
        case OIInstType::Msubs:
        case OIInstType::Msubd:
        case OIInstType::Ext:
          return EncodingType::PL24;
        case OIInstType::Ldihi:
          return EncodingType::PL18i;
        case OIInstType::Add:
        case OIInstType::Sub:
        case OIInstType::Slt:
        case OIInstType::Sltu:
        case OIInstType::And:
        case OIInstType::Or:
        case OIInstType::Nor:
        case OIInstType::Shl:
        case OIInstType::Shlr:
        case OIInstType::Shr:
        case OIInstType::Asr:
        case OIInstType::Asrr:
        case OIInstType::Movn:
        case OIInstType::Movz:
        case OIInstType::Movzd:
        case OIInstType::Movzs:
        case OIInstType::Xor:
        case OIInstType::Adds:
        case OIInstType::Addd:
        case OIInstType::Lwxc1:
        case OIInstType::Swxc1:
        case OIInstType::Muls:
        case OIInstType::Muld:
        case OIInstType::Divs:
        case OIInstType::Divd:
        case OIInstType::Subd:
        case OIInstType::Subs:
        case OIInstType::Sdxc1:
        case OIInstType::Ldxc1:
        case OIInstType::Shrr:
          return EncodingType::PL18;
        case OIInstType::Jlez:
        case OIInstType::Jgtz:
        case OIInstType::Jltz:
        case OIInstType::Jgez:
        case OIInstType::Ldi:
          return EncodingType::PL20;
        case OIInstType::Ijmphi:
          return EncodingType::PL20i;
        case OIInstType::Call:
          return EncodingType::PL26c;
        case OIInstType::Jumpr:
          return EncodingType::PL6;
        case OIInstType::Jump:
          return EncodingType::PL26j;
        case OIInstType::Ijmp:
          return EncodingType::PL26ij;
        case OIInstType::Seh:
        case OIInstType::Seb:
        case OIInstType::Callr:
        case OIInstType::Mtlc1:
        case OIInstType::Mthc1:
        case OIInstType::Ceqs:
        case OIInstType::Ceqd:
        case OIInstType::Movd:
        case OIInstType::Movs:
        case OIInstType::Mtc1:
        case OIInstType::Mfc1:
        case OIInstType::Truncws:
        case OIInstType::Truncwd:
        case OIInstType::Cvtsw:
        case OIInstType::Cvtdw:
        case OIInstType::Cvtds:
        case OIInstType::Cvtsd:
        case OIInstType::Coltd:
        case OIInstType::Colts:
        case OIInstType::Coled:
        case OIInstType::Coles:
        case OIInstType::Culed:
        case OIInstType::Cules:
        case OIInstType::Cults:
        case OIInstType::Cultd:
        case OIInstType::Cund:
        case OIInstType::Negd:
        case OIInstType::Negs:
        case OIInstType::Mflc1:
        case OIInstType::Mfhc1:
        case OIInstType::Movf:
        case OIInstType::Movfd:
        case OIInstType::Movfs:
        case OIInstType::Movtd:
        case OIInstType::Movt:
        case OIInstType::Movnd:
        case OIInstType::Movns:
        case OIInstType::Absd:
        case OIInstType::Abss:
        case OIInstType::Sqrtd:
        case OIInstType::Sqrts:
        case OIInstType::Ror:
          return EncodingType::PL12;
        case OIInstType::Bc1f:
        case OIInstType::Bc1t:
          return EncodingType::PL16;
        case OIInstType::Syscall:
        case OIInstType::Nop:
        case OIInstType::Null:
          return EncodingType::PL0;
      }

      std::cout << "Dammit! We have a bug on Encoding types!\n";
      return EncodingType::PL0;
    }

    static void fillFields(OIInst& I, EncodingType E, Word W) {
      switch (E) {
        case EncodingType::PL26i:
          I.Imm = getImm1(W);
          I.RS = getRS(W);
          I.RT = getRT(W);
          break;
        case EncodingType::PL24:
          I.RV = getRV(W);
          I.RD = getRD(W);
          I.RS = getRS(W);
          I.RT = getRT(W);
          break;
        case EncodingType::PL18i:
          I.Addrs = getPL18(W);
          break;
        case EncodingType::PL18:
          I.RS = getRS(W);
          I.RT = getRT(W);
          I.RD = getRD(W);
          break;
        case EncodingType::PL20:
          I.RT = getRT(W);
          I.Imm = getImm(W);
          break;
        case EncodingType::PL20i:
          I.Addrs = getAddr(W);
          break;
        case EncodingType::PL26c:
          I.Addrs = getAddr(W);
          break;
        case EncodingType::PL6:
          I.RT = getRT(W);
          break;
        case EncodingType::PL26j:
          I.Addrs = getLAddr(W);
          break;
        case EncodingType::PL26ij:
          I.RS  = getCount(W);
          I.RT  = getIndex(W);
          I.Imm = getPL12(W);
          break;
        case EncodingType::PL12:
          I.RS = getRS(W);
          I.RT = getRT(W);
          break;
        case EncodingType::PL16:
          I.Imm = getHalfword(W); 
          break;
        case EncodingType::PL0:
          break;
        default: 
          std::cout << "It may seem unbelievable, but we haven't implemented this encoding type!\n";
          break;
      }
    }

    static OIInst decode(uint32_t CodedInst) {
      Word W;
      W.asI_ = CodedInst;
      uint8_t Op = W.asI_ >> 26;
      constexpr unsigned OpMask = 0x3FFFFFF;

      OIInst I;
      I.Type = OIInstType::Null;

      if (CodedInst == 0) {
        I.Type = OIInstType::Nop;
        return I;
      }

      uint8_t Ext;
      switch(Op) {
      case 0b000000:
        I.Type = OIInstType::Jump;
        break;
      case 0b000001:
        I.Type = OIInstType::Call;
        break;
      case 0b000011:
        I.Type = OIInstType::Ldbu;
        break;
      case 0b000101:
        I.Type = OIInstType::Ldhu;
        break;
      case 0b000110:
        I.Type = OIInstType::Ldw;
        break;
      case 0b001001:
        I.Type = OIInstType::Stb;
        break;
      case 0b001010:
        I.Type = OIInstType::Sth;
        break;
      case 0b001011:
        I.Type = OIInstType::Stw;
        break;
      case 0b001110:
        I.Type = OIInstType::Addi;
        break;
      case 0b010010:
        I.Type = OIInstType::Andi;
        break;
      case 0b011001:
        I.Type = OIInstType::Sdc1;
        break;
      case 0b011011:
        I.Type = OIInstType::Swc1;
        break;
      case 0b011010:
        I.Type = OIInstType::Ldc1;
        break;
      case 0b011100:
        I.Type = OIInstType::Lwc1;
        break;
      case 0b100101:
        I.Type = OIInstType::Mulu;
        break;
      case 0b100110:
        I.Type = OIInstType::Ijmp;
        break;

      case 0b100010:
        Ext = (W.asI_ & OpMask) >> 12;
        if (Ext == 0b0     ) I.Type = OIInstType::Callr;
        if (Ext == 0b11    ) I.Type = OIInstType::Seb;
        if (Ext == 0b100   ) I.Type = OIInstType::Seh;
        if (Ext == 0b101   ) I.Type = OIInstType::Absd;
        if (Ext == 0b110   ) I.Type = OIInstType::Abss;
        if (Ext == 0b111   ) I.Type = OIInstType::Ceqd;
        if (Ext == 0b1000  ) I.Type = OIInstType::Ceqs;
        if (Ext == 0b1001  ) I.Type = OIInstType::Coled;
        if (Ext == 0b1010  ) I.Type = OIInstType::Coles;
        if (Ext == 0b1011  ) I.Type = OIInstType::Coltd;
        if (Ext == 0b1100  ) I.Type = OIInstType::Colts;
        if (Ext == 0b1111  ) I.Type = OIInstType::Culed;
        if (Ext == 0b10000 ) I.Type = OIInstType::Cules;
        if (Ext == 0b10001 ) I.Type = OIInstType::Cultd;
        if (Ext == 0b10010 ) I.Type = OIInstType::Cults;
        if (Ext == 0b10011 ) I.Type = OIInstType::Cund;
        if (Ext == 0b10101 ) I.Type = OIInstType::Cvtsd;
        if (Ext == 0b10110 ) I.Type = OIInstType::Cvtds;
        if (Ext == 0b10111 ) I.Type = OIInstType::Cvtdw;
        if (Ext == 0b11000 ) I.Type = OIInstType::Cvtsw;
        if (Ext == 0b11001 ) I.Type = OIInstType::Mfc1;
        if (Ext == 0b11010 ) I.Type = OIInstType::Movd;
        if (Ext == 0b11011 ) I.Type = OIInstType::Movs;
        if (Ext == 0b11100 ) I.Type = OIInstType::Mtc1;
        if (Ext == 0b11101 ) I.Type = OIInstType::Negd;
        if (Ext == 0b11110 ) I.Type = OIInstType::Negs;
        if (Ext == 0b11111 ) I.Type = OIInstType::Truncwd;
        if (Ext == 0b100000) I.Type = OIInstType::Truncws;
        if (Ext == 0b100001) I.Type = OIInstType::Sqrtd;
        if (Ext == 0b100010) I.Type = OIInstType::Sqrts;
        if (Ext == 0b100011) I.Type = OIInstType::Movtd;
        if (Ext == 0b100101) I.Type = OIInstType::Movfd;
        if (Ext == 0b100110) I.Type = OIInstType::Movfs;
        if (Ext == 0b100111) I.Type = OIInstType::Movf;
        if (Ext == 0b101000) I.Type = OIInstType::Movt;
        if (Ext == 0b101001) I.Type = OIInstType::Mfhc1;
        if (Ext == 0b101010) I.Type = OIInstType::Mthc1;
        if (Ext == 0b110001) I.Type = OIInstType::Mflc1;
        if (Ext == 0b110010) I.Type = OIInstType::Mtlc1;
        break;
      case 0b100000:
        Ext = (W.asI_ & OpMask) >> 18;
        if (Ext == 0b0     ) I.Type = OIInstType::Add;
        if (Ext == 0b1     ) I.Type = OIInstType::Ldihi;
        if (Ext == 0b10    ) I.Type = OIInstType::Sub;
        if (Ext == 0b100   ) I.Type = OIInstType::Slt;
        if (Ext == 0b101   ) I.Type = OIInstType::Sltu;
        if (Ext == 0b110   ) I.Type = OIInstType::And;
        if (Ext == 0b111   ) I.Type = OIInstType::Or;
        if (Ext == 0b1000  ) I.Type = OIInstType::Xor;
        if (Ext == 0b1001  ) I.Type = OIInstType::Nor;
        if (Ext == 0b1010  ) I.Type = OIInstType::Shl;
        if (Ext == 0b1011  ) I.Type = OIInstType::Shr;
        if (Ext == 0b1100  ) I.Type = OIInstType::Asr;
        if (Ext == 0b1101  ) I.Type = OIInstType::Shlr;
        if (Ext == 0b1110  ) I.Type = OIInstType::Shrr;
        if (Ext == 0b1111  ) I.Type = OIInstType::Asrr;
        if (Ext == 0b10000 ) I.Type = OIInstType::Movz;
        if (Ext == 0b10001 ) I.Type = OIInstType::Movn;
        if (Ext == 0b10010 ) I.Type = OIInstType::Ror;
        if (Ext == 0b10100 ) I.Type = OIInstType::Addd;
        if (Ext == 0b10101 ) I.Type = OIInstType::Adds;
        if (Ext == 0b10110 ) I.Type = OIInstType::Divd;
        if (Ext == 0b10111 ) I.Type = OIInstType::Divs;
        if (Ext == 0b11000 ) I.Type = OIInstType::Muld;
        if (Ext == 0b11001 ) I.Type = OIInstType::Muls;
        if (Ext == 0b11010 ) I.Type = OIInstType::Subd;
        if (Ext == 0b11011 ) I.Type = OIInstType::Subs;
        if (Ext == 0b11100 ) I.Type = OIInstType::Movzd;
        if (Ext == 0b11101 ) I.Type = OIInstType::Movzs;
        if (Ext == 0b11110 ) I.Type = OIInstType::Movnd;
        if (Ext == 0b11111 ) I.Type = OIInstType::Movns;
        if (Ext == 0b100000) I.Type = OIInstType::Ldxc1;
        if (Ext == 0b100001) I.Type = OIInstType::Sdxc1;
        if (Ext == 0b100010) I.Type = OIInstType::Lwxc1;
        if (Ext == 0b100011) I.Type = OIInstType::Swxc1;
        break;
      case 0b100001:
        Ext = (W.asI_ & OpMask) >> 16;
        if (Ext == 0b0 ) I.Type = OIInstType::Bc1t;
        if (Ext == 0b10) I.Type = OIInstType::Bc1f;
        break;
      case 0b11110:
        Ext = (W.asI_ & OpMask) >> 24;
        if (Ext == 0b0 ) I.Type = OIInstType::Maddd;
        if (Ext == 0b01) I.Type = OIInstType::Madds;
        if (Ext == 0b10) I.Type = OIInstType::Msubd;
        if (Ext == 0b11) I.Type = OIInstType::Msubs;
        break;
      case 0b11111: 
        Ext = (W.asI_ & OpMask) >> 20;
        if (Ext == 0b0   ) I.Type = OIInstType::Jlez;
        if (Ext == 0b1   ) I.Type = OIInstType::Jgtz;
        if (Ext == 0b10  ) I.Type = OIInstType::Jltz;
        if (Ext == 0b11  ) I.Type = OIInstType::Jgez;
        if (Ext == 0b100 ) I.Type = OIInstType::Ldi;
        if (Ext == 0b10000) I.Type = OIInstType::Ijmphi;
        break;

      case 0b100011:
        I.Type = OIInstType::Jumpr;
        break;
      case 0b10100:
        I.Type = OIInstType::Xori;
        break;
      case 0b10001:
        I.Type = OIInstType::Sltiu;
        break;
      case 0b10000:
        I.Type = OIInstType::Slti;
        break;
      case 0b10101:
        if (getRT(W) == 0) 
          I.Type = OIInstType::Jeqz;
        else
          I.Type = OIInstType::Jeq;
        break;
      case 0b10110:
        if (getRT(W) == 0) 
          I.Type = OIInstType::Jnez;
        else
          I.Type = OIInstType::Jne;
        break;
      case 0b011101: 
        Ext = (W.asI_ & OpMask) >> 24;
        if (Ext == 0b00) I.Type = OIInstType::Mul;
        if (Ext == 0b01) {
          if (getRD(W) != 0)
            I.Type = OIInstType::Div;
          if (getRV(W) != 0)
            I.Type = OIInstType::Mod;
        }
        if (Ext == 0b10) {
          if (getRD(W) != 0)
            I.Type = OIInstType::Divu;
          if (getRV(W) != 0)
            I.Type = OIInstType::Modu;
        }
        if (Ext == 0b11) I.Type = OIInstType::Ext;
        break;
      case 0b100100:
        I.Type = OIInstType::Syscall;
        break;
      case 0b10011: // FIXME: Is this a UImm?
        I.Type = OIInstType::Ori;
        break;
      case 0b10:
        I.Type = OIInstType::Ldb;
        break;
      case 0b100:
        I.Type = OIInstType::Ldh;
        break;
      default:
        I.Type = OIInstType::Null;
        break;
      }

      if (I.Type == OIInstType::Null) {
        std::cout << "Houston: we have a problem! Inst (" << std::hex << CodedInst << ") not implemented!\n";
        exit(1);
      }

      fillFields(I, getEncodingType(I.Type), W);

      return I;
    }


    static std::array<uint32_t, 2> getPossibleTargets(uint32_t PC, OIInst Inst) {
      switch (Inst.Type) {
        case dbt::OIDecoder::Jne: 
        case dbt::OIDecoder::Jeqz: 
        case dbt::OIDecoder::Jlez:
        case dbt::OIDecoder::Jltz:
        case dbt::OIDecoder::Jnez:
        case dbt::OIDecoder::Jgtz:
        case dbt::OIDecoder::Jeq: 
          return {(PC + (Inst.Imm << 2) + 4), PC + 4};
        case dbt::OIDecoder::Jump: 
        case dbt::OIDecoder::Call: 
          return {(PC & 0xF0000000) | (Inst.Addrs << 2), 0};
        default:
          return {PC, PC};
      }
      return {PC, PC};
    }

    static bool isControlFlowInst(OIInst Inst) {
      auto T = getPossibleTargets(1, Inst);
      if (T[0] == 1 && T[1] == 1) 
        return false;
      return true;
    }

  }
}

#endif
