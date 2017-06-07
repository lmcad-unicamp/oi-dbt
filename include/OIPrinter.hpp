#ifndef OIPRINTER_H
#define OIPRINTER_H

#include <OIDecoder.hpp>

#include <sstream>

using namespace dbt::OIDecoder;

namespace dbt {
  namespace OIPrinter {
    static std::string getString(OIInst I) {
      std::ostringstream Out;
      switch (I.Type) {
        case Add:
          Out << "add";
          break;
        case Sub:
          Out << "sub";
          break;
        case And:
          Out << "and";
          break;
        case Andi:
          Out << "andi";
          break;
        case Or:
          Out << "or";
          break;
        case Nor:
          Out << "Nor";
          break;
        case Ldh:
          Out << "ldh";
          break;
        case Ldi:
          Out << "ldi";
          break;
        case Ldihi:
          Out << "ldihi";
          break;
        case Ldw:
          Out << "ldw";
          break;
        case Addi:
          Out << "addi";
          break;
        case Call:
          Out << "call";
          break;
        case Callr:
          Out << "callr";
          break;
        case Jumpr:
          Out << "jumpr";
          break;
        case Stw:
          Out << "stw";
          break;
        case Sltiu:
          Out << "sltiu";
          break;
        case Slti:
          Out << "slti";
          break;
        case Slt:
          Out<< "slt";
          break;
        case Jeq:
          Out << "jeq";
          break;
        case Jeqz:
          Out << "jeqz";
          break;
        case Jne:
          Out << "jne";
          break;
        case Jnez:
          Out << "jnez";
          break;
        case Jump:
          Out << "jump";
          break;
        case Mul:
          Out << "mul";
          break;
        case Mulu:
          Out << "mulu";
          break;
        case Div:
          Out << "div";
          break;
        case Mod:
          Out << "mod";
          break;
        case Syscall:
          Out << "sycall";
          break;
        case Nop:
          Out << "nop";
          break;
        case Shr:
          Out << "shr";
          break;
        case Shl:
          Out << "shl";
          break;
        case Shlr:
          Out << "shlr";
          break;
        case Ori:
          Out << "ori";
          break;
        case Jgtz:
          Out << "jgtz";
          break;
        case Jlez:
          Out << "jlez";
          break;
        case Jltz:
          Out << "jltz";
          break;
        case Stb:
          Out << "stb";
          break;
        case Ldb:
          Out << "ldb";
          break;
        case Ldbu:
          Out << "ldbu";
          break;
        case Sltu:
          Out << "sltu";
          break;
        case Asr:
          Out << "asr";
          break;
        case Movn:
          Out << "movn";
          break;
        case Movz:
          Out << "movz";
          break;
        case Xori:
          Out << "xori";
          break;
        case Xor:
          Out << "xor";
          break;
        case Jgez:
          Out << "Jgez";
          break;
        case Sth:
          Out << "sth";
          break;
        case Ldhu:
          Out << "ldhu";
          break;
        case Seh:
          Out << "Seh";
          break;
        default:
          Out << "null";
          break;
      }

      switch (dbt::OIDecoder::getEncodingType(I.Type)) {
        case EncodingType::PL26i:
          if (I.Type == OIInstType::Ldw || I.Type == OIInstType::Stw) 
            Out << "  $" << (uint32_t) I.RT << ", " << I.Imm << "($" << (uint32_t) I.RS << ")";
          else if (I.Type == OIInstType::Jne || I.Type == OIInstType::Jnez || I.Type == OIInstType::Jeq || I.Type == OIInstType::Jeqz) 
            Out << "  $" << (uint32_t) I.RT << ", $" << (uint32_t) I.RS << ", " << ((I.Imm << 2) + 4);
          else
            Out << "  $" << (uint32_t) I.RS << ", $" << (uint32_t) I.RT << ", " << I.Imm;
          break;
        case EncodingType::PL24:
          Out << "  $" << (uint32_t) I.RT << ", $" << (uint32_t) I.RD << ", $" << (uint32_t) I.RS << ", $" << (uint32_t) I.RV;
          break;
        case EncodingType::PL18i:
          Out << "  " << I.Addrs;
          break;
        case EncodingType::PL18:
          Out << "  $" << (uint32_t) I.RT << ", $" << (uint32_t) I.RS << ", $" << (uint32_t) I.RD;
          break;
        case EncodingType::PL12:
          Out << "  $" << (uint32_t) I.RT << ", $" << (uint32_t) I.RS;
          break;
        case EncodingType::PL20:
          if (I.Type == OIInstType::Jlez || I.Type == OIInstType::Jgtz || I.Type == OIInstType::Jltz || I.Type == OIInstType::Jgez) 
            Out << " $" << (uint32_t) I.RT << ", " << ((I.Imm << 2) + 4);
          else
            Out << " $" << (uint32_t) I.RT << ", " << I.Imm;
          break;
        case EncodingType::PL26c:
          Out << " " << I.Addrs;
          break;
        case EncodingType::PL6:
          Out << " $" << (uint32_t) I.RT;
          break;
        case EncodingType::PL26j:
          Out << " " << I.Addrs;
          break;
        case EncodingType::PL0:
        default: 
          break;
      }

      return Out.str();
    }

  };
};

#endif
