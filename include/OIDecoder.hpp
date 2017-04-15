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
    { Add, And, Andi, Or, Ldi, Ldihi, Ldw, Addi, Call, Jumpr, Stw, Sltiu, Slti, Jeq, Jne, Jump, Mul, Syscall, Nop, 
      Shr, Shl, Jeqz, Sub, Slt, Div, Mod, Ori, Jgtz, Jlez, Jnez, Ldbu, Stb, Null };

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

    static OIInst decode(uint32_t CodedInst) {
      Word W;
      W.asI_ = CodedInst;
      uint8_t Op = W.asI_ >> 26;
      constexpr unsigned OpMask = 0x3FFFFFF;

      OIInst I;

      if (CodedInst == 0) {
        I.Type = OIInstType::Nop;
        return I;
      }

      uint8_t Ext;
      switch(Op) {
      case 0b10010:
        I.Type = OIInstType::Andi;
        I.Imm = getImm1(W);
        I.RS = getRS(W);
        I.RT = getRT(W);
        break;
      case 0b1001:
        I.Type = OIInstType::Stb;
        I.Imm = getImm1(W);
        I.RS = getRS(W);
        I.RT = getRT(W);
      case 0b11:
        I.Type = OIInstType::Ldbu;
        I.Imm = getImm1(W);
        I.RS = getRS(W);
        I.RT = getRT(W);
        break;
      case 0b100000:
        Ext = W.asI_ >> 18;
        if (Ext == 0b0) I.Type = OIInstType::Add;
        if (Ext == 0b1) {
          I.Type = OIInstType::Ldihi;
          I.Addrs = getPL18(W);
        }
        if (Ext == 0b10) I.Type = OIInstType::Sub;
        if (Ext == 0b100) I.Type = OIInstType::Slt;
        if (Ext == 0b110) I.Type = OIInstType::And;
        if (Ext == 0b111) I.Type = OIInstType::Or;
        if (Ext == 0b1010) I.Type = OIInstType::Shl;
        if (Ext == 0b1011) I.Type = OIInstType::Shr;
        I.RS = getRS(W);
        I.RT = getRT(W);
        I.RD = getRD(W);
        break;
      case 0b11111: 
        Ext = (W.asI_ & OpMask) >> 20;
        if (Ext == 0b1) I.Type = OIInstType::Jgtz;
        if (Ext == 0b0) I.Type = OIInstType::Jlez;
        if (Ext == 0b100) I.Type = OIInstType::Ldi;
        I.RT = getRT(W);
        I.Imm = getImm(W);
        break;
      case 0b110:
        I.Type = OIInstType::Ldw;
        I.RS = getRS(W);
        I.RT = getRT(W);
        I.Imm = getImm1(W);
        break;
      case 0b1110:
        I.Type = OIInstType::Addi;
        I.RT = getRT(W);
        I.RS = getRS(W);
        I.Imm = getImm1(W);
        break;
      case 0b1:
        I.Type = OIInstType::Call;
        I.Addrs = getAddr(W);
        break;
      case 0b100011:
        I.Type = OIInstType::Jumpr;
        I.RT = getRT(W);
        break;
      case 0b1011:
        I.Type = OIInstType::Stw;
        I.RS = getRS(W);
        I.RT = getRT(W);
        I.Imm = getImm1(W);
        break;
      case 0b10001:
        I.Type = OIInstType::Sltiu;
        I.RS = getRS(W);
        I.RT = getRT(W);
        I.Imm = getImm1(W);
        break;
      case 0b10000:
        I.Type = OIInstType::Slti;
        I.RS = getRS(W);
        I.RT = getRT(W);
        I.Imm = getImm1(W);
        break;
      case 0b10101:
        I.RS = getRS(W);
        I.RT = getRT(W);
        I.Imm = getImm1(W);

        if (I.RT == 0) 
          I.Type = OIInstType::Jeqz;
        else
          I.Type = OIInstType::Jeq;

        break;
      case 0b10110:
        I.RS = getRS(W);
        I.RT = getRT(W);
        I.Imm = getImm1(W);

        if (I.RT == 0) 
          I.Type = OIInstType::Jnez;
        else
          I.Type = OIInstType::Jne;

        break;
      case 0b0:
        I.Type = OIInstType::Jump;
        I.Addrs = getLAddr(W);
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
        I.RV = getRV(W);
        I.RD = getRD(W);
        I.RS = getRS(W);
        I.RT = getRT(W);
        break;
      case 0b100100:
        I.Type = OIInstType::Syscall;
        break;
      case 0b10011:
        I.Type = OIInstType::Ori;
        I.Imm = getUImm1(W);
        I.RS  = getRS(W);
        I.RT  = getRT(W);
        break;
      default:
        std::cout << "Houston: we have a problem! Inst (" << std::hex << CodedInst << ") not implemented!\n";
        I.Type = OIInstType::Null;
        break;
      }
      return I;
    }
  }
}

#endif
