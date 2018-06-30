#include <syscall.hpp>

#include <iostream>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <ostream>
#include <ctime>

//#ifdef DUDECT
#include <dudect.hpp>
//#endif

using namespace dbt;
using namespace dut;

static Dudect* dudect = NULL;
std::ofstream DudectFile;

int LinuxSyscallManager::processSyscall(Machine& M) {

  if(!dudect)
  {
    dudect = new Dudect(M.getDudectMeasures());
    std::cout << "Dudect: Number of measures is set to " << M.getDudectMeasures() << std::endl;

    DudectFile.open(M.getDudect_ReportPath(), std::ofstream::out | std::ofstream::app);

    std::cout << "Dudect: Output Report Path " << M.getDudect_ReportPath() << std::endl;

    DudectFile << "\nDUDECT INITIALIZING\n" << std::endl;
  }

  SyscallType SysTy = static_cast<SyscallType>(M.getRegister(4) - 4000);

  switch (SysTy) {
  case SyscallType::Exit:
    ExitStatus = M.getRegister(2);
    std::cerr << "Exiting with status " << (uint32_t) ExitStatus << " (" << M.getRegister(2) << ")\n";
    return 1;
  case SyscallType::Fstat: {
    //int r = fstat(M.getRegister(5), (struct stat*) (M.getByteMemoryPtr() + (M.getRegister(6) - M.getDataMemOffset())));
    //M.setRegister(2, r);
		M.setRegister(2, -1);
    return 0;
  }

  //GET/PUT right registers and memory locations
  case SyscallType::Read:{
    //fflush(stdin);
    ssize_t r = read(M.getRegister(5), (M.getByteMemoryPtr() + (M.getRegister(6) - M.getDataMemOffset())), M.getRegister(7));
    M.setRegister(2, r);
    return 0;
  }
  case SyscallType::Write: {
    ssize_t r = write(M.getRegister(5), (M.getByteMemoryPtr() + (M.getRegister(6) - M.getDataMemOffset())), M.getRegister(7));
    M.setRegister(2, r);
    return 0;
  }

  case SyscallType::Open: {
    const char* filename = M.getByteMemoryPtr() + (M.getRegister(5) - M.getDataMemOffset());
    const int flags = M.getRegister(6);

    ssize_t r = -1;
    if (flags == 0 || flags == 1 || flags == 2)
      r = open(filename, flags);
    else
	   r = open(filename, flags | O_CREAT, S_IRWXU);

    M.setRegister(2, r);

    #ifdef DEBUG
    std::cerr << "Open file: " << filename << "; Flags:" << flags << " (r=" << r << ")" << std::endl;
    assert(r >= 0 && "Error with file descriptor..");
    #endif

    return 0;
  }

  case SyscallType::Close: {
    auto FD = M.getRegister(5);
    if (FD > 2) {
      ssize_t r = close(FD);
      M.setRegister(2, r);
    } else {
      M.setRegister(2, 0);
    }
    return 0;
  }

  case SyscallType::Creat: {
    const char* filename = M.getByteMemoryPtr() + (M.getRegister(5) - M.getDataMemOffset());
    const int flags = M.getRegister(6);

    ssize_t r = creat(filename, flags);
    M.setRegister(2, r);

    assert(r >= 0 && "Error with file descriptor..");
    return 0;
  }

  //Hack for rtdsc instruction
  case SyscallType::Stat: {
    static int64_t initialTime = 0, finalTime = 0;
    static bool started = false, initialized = false;
    static uint32_t times = 0;
    const char* classe = M.getByteMemoryPtr() + (M.getRegister(5) - M.getDataMemOffset());

    finalTime = cpucycles();

    if(classe[0] == 3)
    {
      //std::cout << "Class 3\n";
      if(initialized)
        dudect->doit();

      initialized = true;

      dudect->resetData();
      initialTime = finalTime = started = 0x0;
    }

    else if(!started)
    {
      //std::cout << "Started\n";
      started = true;
    }

    else if(initialTime == 0)
    {
      //std::cout << "Iniatialized!\n";
      initialTime = finalTime;
    }

    else
    {
      //std::cout << "Measure\n";
      if(dudect->measure(finalTime-initialTime, (uint8_t) classe[0]) == true)
        times++;
      else
      {
        dudect->doit();
        dudect->resetData();
      }
        //std::cout << "[" << times << "]" <<  ": " << finalTime-initialTime << std::endl;
      DudectFile << "[" << times << "]" <<  ": " << finalTime-initialTime << std::endl;
    }

    return 0;
  }
  case SyscallType::Lseek: {
    //const char* filename = M.getByteMemoryPtr() + (M.getRegister(5) - M.getDataMemOffset());
    //const int flags = M.getRegister(6);

    ssize_t r = lseek(M.getRegister(5), M.getRegister(6), M.getRegister(7));
    M.setRegister(2, r);

    assert(r >= 0 && "Error with file descriptor..");
    return 0;
  }

  default:
    std::cerr << "Syscall (" << SysTy << ") not implemented!\n";
    exit(2);
    break;
  }
  return 0;
}
