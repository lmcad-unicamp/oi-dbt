#ifndef SYSCALLS_H
#define SYSCALLS_H

#include <machine.hpp>

namespace dbt {
  class SyscallManager {
  protected:
    uint8_t ExitStatus;
  public:
    uint8_t getExitStatus() { return ExitStatus; };

    virtual int processSyscall(Machine&) = 0;
  };

  class LinuxSyscallManager : public SyscallManager {
    public:
      enum SyscallType { Exit = 1, Read=0x3, Write = 0x4, Open=0x5, Close=0x6, Creat=0x8, Lseek=0x13, Stat=106, Fstat = 108 };
      int processSyscall(Machine&);
  };
}

#endif
