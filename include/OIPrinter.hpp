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
        case Absd:
          Out << "abs.d";
          break;
        case Abss:
          Out << "abs.s";
          break;
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
        case Divu:
          Out << "divu";
          break;
        case Modu:
          Out << "modu";
          break;
        case Syscall:
          Out << "syscall";
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
        case Shrr:
          Out << "shrr";
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
        case Asrr:
          Out << "asrr";
          break;
        case Movn:
          Out << "movn";
          break;
        case Movz:
          Out << "movz";
          break;
        case Ror:
          Out << "ror";
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
          Out << "seh";
          break;
        case Seb:
          Out << "seb";
          break;
        case Ijmp:
          Out << "ijmp";
          break;
        case Ijmphi:
          Out << "ijmphi";
          break;
        case Ldc1:
          Out << "ldc1";
          break;
        case Sdc1:
          Out << "sdc1";
          break;
        case Ldxc1:
          Out << "ldxc1";
          break;
        case Sdxc1:
          Out << "sdxc1";
          break;
        case Mtlc1:
          Out << "Mtlc1";
          break;
        case Mthc1:
          Out << "Mthc1";
          break;
        case Ceqs:
          Out << "c.eq.s";
          break;
        case Cueqd:
          Out << "c.ueq.d";
          break;
        case Ceqd:
          Out << "c.eq.d";
          break;
        case Bc1f:
          Out << "bc1f";
          break;
        case Bc1t:
          Out << "bc1t";
          break;
        case Movs:
          Out << "mov.s";
          break;
        case Movzd:
          Out << "movz.d";
          break;
        case Movzs:
          Out << "movz.s";
          break;
        case Movnd:
          Out << "movn.d";
          break;
        case Movns:
          Out << "movn.s";
          break;
        case Movtd:
          Out << "movt.d";
          break;
        case Movts:
          Out << "movt.s";
          break;
        case Movfd:
          Out << "movf.d";
          break;
        case Movfs:
          Out << "movf.s";
          break;
        case Movd:
          Out << "mov.d";
          break;
        case Movf:
          Out << "movf";
          break;
        case Movt:
          Out << "movt";
          break;
        case Lwc1:
          Out << "lwc1";
          break;
        case Adds:
          Out << "add.s";
          break;
        case Addd:
          Out << "add.d";
          break;
        case Mtc1:
          Out << "mtc1";
          break;
        case Mfc1:
          Out << "mfc1";
          break;
        case Truncws:
          Out << "trunc.w.s";
          break;
        case Truncwd:
          Out << "trunc.w.d";
          break;
        case Cvtsw:
          Out << "cvt.s.w";
          break;
        case Cvtdw:
          Out << "cvt.d.w";
          break;
        case Cvtds:
          Out << "cvt.d.s";
          break;
        case Cvtsd:
          Out << "cvt.s.d";
          break;
        case Lwxc1:
          Out << "lwxc1";
          break;
        case Swc1:
          Out << "swc1";
          break;
        case Swxc1:
          Out << "swxc1";
          break;
        case Muls:
          Out << "mul.s";
          break;
        case Muld:
          Out << "mul.d";
          break;
        case Coltd:
          Out << "c.olt.d";
          break;
        case Colts:
          Out << "c.olt.s";
          break;
        case Coled:
          Out << "c.ole.d";
          break;
        case Coles:
          Out << "c.ole.s";
          break;
        case Culed:
          Out << "c.ule.d";
          break;
        case Cules:
          Out << "c.ule.s";
          break;
        case Cults:
          Out << "c.ult.s";
          break;
        case Cultd:
          Out << "c.ult.d";
          break;
        case Cund:
          Out << "C.un.d";
          break;
        case Cuns:
          Out << "C.un.s";
          break;
        case Divs:
          Out << "div.s";
          break;
        case Divd:
          Out << "div.d";
          break;
        case Negd:
          Out << "neg.d";
          break;
        case Negs:
          Out << "neg.s";
          break;
        case Mflc1:
          Out << "mflc1";
          break;
        case Subs:
          Out << "sub.s";
          break;
        case Subd:
          Out << "sub.d";
          break;
        case Mfhc1:
          Out << "mfhc1";
          break;
        case Msubs:
          Out << "msub.s";
          break;
        case Msubd:
          Out << "msub.d";
          break;
        case Madds:
          Out << "madd.s";
          break;
        case Maddd:
          Out << "madd.d";
          break;
        case Sqrts:
          Out << "sqrt.s";
          break;
        case Sqrtd:
          Out << "sqrt.d";
          break;
        case Ext:
          Out << "ext";
          break;
        default:
          Out << "null";
          break;
      }

      switch (dbt::OIDecoder::getEncodingType(I.Type)) {
        // TODO PL26ij and PL20i
        case EncodingType::PL26i:
          if (I.Type == OIInstType::Ldw || I.Type == OIInstType::Stw)
            Out << "  $" << (uint32_t) I.RT << ", " << I.Imm << "($" << (uint32_t) I.RS << ")";
          else if (I.Type == OIInstType::Jne || I.Type == OIInstType::Jnez || I.Type == OIInstType::Jeq || I.Type == OIInstType::Jeqz)
            Out << "  $" << (uint32_t) I.RT << ", $" << (uint32_t) I.RS << ", " << ((I.Imm << 2) + 4);
          else
            Out << "  $" << (uint32_t) I.RT << ", $" << (uint32_t) I.RS << ", " << I.Imm;
          break;
        case EncodingType::PL24:
          Out << "  $" << (uint32_t) I.RT << ", $" << (uint32_t) I.RD << ", $" << (uint32_t) I.RS << ", $" << (uint32_t) I.RV;
          break;
        case EncodingType::PL18i:
          Out << "  " << I.Addrs;
          break;
        case EncodingType::PL18:
          Out << "  $" << (uint32_t) I.RD << ", $" << (uint32_t) I.RS << ", $" << (uint32_t) I.RT;
          break;
        case EncodingType::PL12:
          Out << "  $" << (uint32_t) I.RS << ", $" << (uint32_t) I.RT;
          break;
        case EncodingType::PL20:
          if (I.Type == OIInstType::Jlez || I.Type == OIInstType::Jgtz || I.Type == OIInstType::Jltz || I.Type == OIInstType::Jgez)
            Out << " $" << (uint32_t) I.RT << ", " << ((I.Imm << 2) + 4);
          else
            Out << " $" << (uint32_t) I.RT << ", " << I.Imm;
          break;
        case EncodingType::PL26c:
          Out << " " << std::hex << I.Addrs << std::dec;
          break;
        case EncodingType::PL6:
          Out << " $" << (uint32_t) I.RT;
          break;
        case EncodingType::PL26j:
          Out << " " << (I.Addrs << 2);
          break;
        case EncodingType::PL16:
          Out << " " << ((I.Imm << 2) + 4);
          break;
        case EncodingType::PL0:
        default:
          std::cout << "---";
          break;
      }

      return Out.str();
    }

  }
}

#endif
