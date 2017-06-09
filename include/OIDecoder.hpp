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
    { Add, And, Andi, Or , Ldi , Ldihi, Ldw, Addi, Call , Jumpr, Stw , Sltiu, Slti, Jeq , Jne, Jump, Mul , Mulu, Syscall, 
      Shr, Shl, Jeqz, Sub, Slt , Div  , Mod, Ori , Jgtz , Jlez , Jnez, Ldbu , Stb , Sltu, Asr, Jltz, Movn, Movz, Xori   ,
      Nor, Ldh, Ldb , Sth, Ldhu, Jgez , Nop, Seh , Callr, Shlr , Xor , Null };

    enum EncodingType {
      PL0, PL6, PL26j, PL26c, PL26i, PL12, PL18, PL16, PL24, PL18i, PL20 
    };

    typedef struct OIInst {
      OIInstType Type;
      uint8_t RS, RT, RD, RV;
      int16_t Imm;
      uint32_t Addrs;
    } OIInst;

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

    static int16_t getImm1(Word W) {
      uint16_t x = (W.asI_ >> 12) & 0x3FFF;
      return (x & 0x2000) ? (x | 0xc000) : x;
    }

    static uint16_t getUImm1(Word W) {
      return (W.asI_ >> 12) & 0x3FFF;
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
          return EncodingType::PL26i;
        case OIInstType::Mulu:
        case OIInstType::Mul:
        case OIInstType::Div:
        case OIInstType::Mod:
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
        case OIInstType::Movn:
        case OIInstType::Movz:
        case OIInstType::Xor:
          return EncodingType::PL18;
        case OIInstType::Jlez:
        case OIInstType::Jgtz:
        case OIInstType::Jltz:
        case OIInstType::Jgez:
        case OIInstType::Ldi:
          return EncodingType::PL20;
        case OIInstType::Call:
          return EncodingType::PL26c;
        case OIInstType::Jumpr:
          return EncodingType::PL6;
        case OIInstType::Jump:
          return EncodingType::PL26j;
        case OIInstType::Seh:
        case OIInstType::Callr:
          return EncodingType::PL12;
        case OIInstType::Syscall:
        case OIInstType::Null:
          return EncodingType::PL0;
        defaut:
          std::cout << "Dammit! We have a bug on Encoding types!\n";
          return EncodingType::PL0;
      }
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
        case EncodingType::PL26c:
          I.Addrs = getAddr(W);
          break;
        case EncodingType::PL6:
          I.RT = getRT(W);
          break;
        case EncodingType::PL26j:
          I.Addrs = getLAddr(W);
          break;
        case EncodingType::PL12:
          I.RS = getRS(W);
          I.RT = getRT(W);
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
      case 0b000101:
        I.Type = OIInstType::Ldhu;
        break;
      case 0b10010:
        I.Type = OIInstType::Andi;
        break;
      case 0b001010:
        I.Type = OIInstType::Sth;
        break;
      case 0b100101:
        I.Type = OIInstType::Mulu;
        break;
      case 0b1001:
        I.Type = OIInstType::Stb;
        break;
      case 0b11:
        I.Type = OIInstType::Ldbu;
        break;
      case 0b100010:
        Ext = (W.asI_ & OpMask) >> 12;
        if (Ext == 0b0  ) I.Type = OIInstType::Callr;
        if (Ext == 0b100) I.Type = OIInstType::Seh;
        break;
      case 0b100000:
        Ext = (W.asI_ & OpMask) >> 18;
        if (Ext == 0b0    ) I.Type = OIInstType::Add;
        if (Ext == 0b1    ) I.Type = OIInstType::Ldihi;
        if (Ext == 0b10   ) I.Type = OIInstType::Sub;
        if (Ext == 0b100  ) I.Type = OIInstType::Slt;
        if (Ext == 0b101  ) I.Type = OIInstType::Sltu;
        if (Ext == 0b110  ) I.Type = OIInstType::And;
        if (Ext == 0b111  ) I.Type = OIInstType::Or;
        if (Ext == 0b1000 ) I.Type = OIInstType::Xor;
        if (Ext == 0b1001 ) I.Type = OIInstType::Nor;
        if (Ext == 0b1010 ) I.Type = OIInstType::Shl;
        if (Ext == 0b1011 ) I.Type = OIInstType::Shr;
        if (Ext == 0b1100 ) I.Type = OIInstType::Asr;
        if (Ext == 0b1101 ) I.Type = OIInstType::Shlr;
        if (Ext == 0b10000) I.Type = OIInstType::Movz;
        if (Ext == 0b10001) I.Type = OIInstType::Movn;
        break;
      case 0b11111: 
        Ext = (W.asI_ & OpMask) >> 20;
        if (Ext == 0b0  ) I.Type = OIInstType::Jlez;
        if (Ext == 0b1  ) I.Type = OIInstType::Jgtz;
        if (Ext == 0b10 ) I.Type = OIInstType::Jltz;
        if (Ext == 0b11 ) I.Type = OIInstType::Jgez;
        if (Ext == 0b100) I.Type = OIInstType::Ldi;
        break;
      case 0b110:
        I.Type = OIInstType::Ldw;
        break;
      case 0b1110:
        I.Type = OIInstType::Addi;
        break;
      case 0b1:
        I.Type = OIInstType::Call;
        break;
      case 0b100011:
        I.Type = OIInstType::Jumpr;
        break;
      case 0b1011:
        I.Type = OIInstType::Stw;
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
      case 0b0:
        I.Type = OIInstType::Jump;
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
          return {(PC + (Inst.Imm << 2)) + 4, PC + 4};
        case dbt::OIDecoder::Jump: 
        case dbt::OIDecoder::Call: 
          return {(PC & 0xF0000000) | (Inst.Addrs << 2), 0};
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
