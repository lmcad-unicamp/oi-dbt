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
  private:
    enum SyscallType { Exit = 1 };
  public:
    int processSyscall(Machine&);
  };
}

#endif
