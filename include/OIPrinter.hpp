#ifndef OIPRINTER_H
#define OIPRINTER_H

#include <OIDecoder.hpp>

#include <sstream>

using namespace dbt::OIDecoder;

namespace dbt {
  namespace OIPrinter {
    static std::string getString(OIInst Inst) {
      std::ostringstream Out;
      switch (Inst.Type) {
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
      case Jgez:
        Out << "Jgez";
        break;
      default:
        Out << "null";
        break;
      }
      return Out.str();
    }
  };
};

#endif
