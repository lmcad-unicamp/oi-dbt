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
      case Stb:
        Out << "stb";
        break;
      case Ldbu:
        Out << "ldbu";
        break;
      default:
        Out << "Null";
        break;
      }
      return Out.str();
    }
  };
};

#endif
