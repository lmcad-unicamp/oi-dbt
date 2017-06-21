#include <syscall.hpp>

#include <iostream>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

using namespace dbt;

int LinuxSyscallManager::processSyscall(Machine& M) {
  SyscallType SysTy = static_cast<SyscallType>(M.getRegister(4) - 4000);

  switch (SysTy) {
  case SyscallType::Exit:
    ExitStatus = M.getRegister(2);
    std::cerr << "Exiting with status " << (uint32_t) ExitStatus << " (" << M.getRegister(2) << ")\n"; 
    return 1; 
  case SyscallType::Fstat: {
    int r = fstat(M.getRegister(5), (struct stat*) (M.getByteMemoryPtr() + (M.getRegister(6) - M.getDataMemOffset())));
		M.setRegister(2, r);
    return 0; 
  } 
  case SyscallType::Write: {
    ssize_t r = write(M.getRegister(5), (M.getByteMemoryPtr() + (M.getRegister(6) - M.getDataMemOffset())), M.getRegister(7));
    M.setRegister(2, r);
    return 0;
  }
  default:
    std::cerr << "Syscall (" << SysTy << ") not implemented!\n";
    exit(2);
    break;
  }
  return 0;
}
