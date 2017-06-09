#include <syscall.hpp>

#include <iostream>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

using namespace dbt;

int LinuxSyscallManager::processSyscall(Machine& M) {
  SyscallType SysTy = static_cast<SyscallType>(M.getRegister(4) - 4000);

  switch (SysTy) {
  case SyscallType::Exit:
    ExitStatus = M.getRegister(2);
    return 1; 
  case SyscallType::Fstat: {
    int r = fstat(M.getRegister(5), (struct stat*) (M.getByteMemoryPtr() + (M.getRegister(6) - M.getDataMemOffset())));
		M.setRegister(2, r);
    return 0; 
  } default:
    std::cout << "Syscall (" << SysTy << ") not implemented!\n";
    exit(2);
    break;
  }
  return 0;
}
