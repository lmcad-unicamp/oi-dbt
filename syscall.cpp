#include <syscall.hpp>

#include <iostream>
#include <signal.h>

using namespace dbt;

int LinuxSyscallManager::processSyscall(Machine& M) {
  SyscallType SysTy = static_cast<SyscallType>(M.getRegister(4) - 4000);

  switch (SysTy) {
  case SyscallType::Exit:
    ExitStatus = M.getRegister(2);
    return 1; 
  default:
    std::cout << "Syscall (" << SysTy << ") not implemented!\n";
    exit(2);
    break;
  }
  return 0;
}
