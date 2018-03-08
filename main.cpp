#include <arglib/arglib.hpp>
#include <interpreter.hpp>
#include <RFT.hpp>
#include <manager.hpp>
#include <syscall.hpp>
#include <timer.hpp>

#include <iostream>
#include <memory>

clarg::argString RFTFlag("-rft", "Region Formation Technique (net)", "net");
clarg::argInt    HotnessFlag("-hot", "Hotness threshold for the RFTs", 50);
clarg::argString ReportFileFlag("-report", "Write down report to a file", "");
clarg::argBool   InterpreterFlag("-interpret",  "Only interpret.");
clarg::argString BinaryFlag("-bin",  "Path to the binary which will should be emulated.", "");
clarg::argBool   PreheatFlag("-p",  "Run one time to compile all regions and then reexecute measuring the time.");
clarg::argBool   VerboseFlag("-v",  "display the compiled regions");
clarg::argBool   HelpFlag("-h",  "display the help message");
clarg::argInt    RegionLimitSize("-l", "region size limit", 0);
clarg::argString ToCompileFlag("-tc", "Functions to compile", "");

void usage(char* PrgName) {
  cout << "Version: 0.0.1 (07-02-2018)\n\n";

  cout << "Usage: " << PrgName << 
    " [-rft {net, mret2, lef, lei, netplus}] [-interpreter] -bin PathToBinary\n\n";

  cout << "DESCRIPTION:\n";
  cout << "This program implements the OpenISA DBT (Dynamic Binary Translator)\n" <<
    "Institute of Computing, 2018.\n\n";

  cout << "ARGUMENTS:\n";
  clarg::arguments_descriptions(cout, "  ", "\n");
}

int validateArguments() {
  if (InterpreterFlag.was_set() && RFTFlag.was_set()) {
    cerr << "You can't use a RFT when you are only interpreting!\n";
    return 1;
  }

  if (!BinaryFlag.was_set()) {
    cerr << "You must set the path of the binary which will be emulated!\n";
    return 1;
  }

  return 0;
}

std::unique_ptr<dbt::RFT> RftChosen;
dbt::Machine M;

void  sigHandler(int sig) {
  if (VerboseFlag.was_set())
    RftChosen->printRegions();

  if (M.isOnNativeExecution()) {
    std::cerr << "Error while executing region " << std::hex << M.getRegionBeingExecuted() << "\n"; 
  } else {
    std::cerr << "Error while executing the interpreter.\n";
    if (M.getRegionBeingExecuted() != 0)
      std::cerr << "The last executed region was " << std::hex << M.getRegionBeingExecuted() << "\n";
  }

  if(sig == SIGABRT)
    std::cerr << "SIGABRT (" << sig << ") while emulating at PC: " << std::hex << M.getPC() << std::dec << "\n";
  else
    std::cerr << "SIGSEGV (" << sig << ") while emulating at PC: " << std::hex << M.getPC() << std::dec << "\n";
  exit(1);
}

int main(int argc, char** argv) {
  signal(SIGSEGV, sigHandler);
  signal(SIGABRT, sigHandler);

  dbt::Timer GlobalTimer; 

  // Parse the arguments
  if (clarg::parse_arguments(argc, argv)) {
    cerr << "Error when parsing the arguments!" << endl;
    return 1;
  }

  if (HelpFlag.get_value() == true) {
    usage(argv[0]);
    return 1;
  }

  if (validateArguments())
    return 1;

  int loadStatus = M.loadELF(BinaryFlag.get_value());

  if (!loadStatus) {
    std::cerr << "Can't find or process ELF file " << argv[1] << std::endl;
    return 2;
  }

  dbt::Manager TheManager(1, dbt::Manager::OptPolitic::Normal, M.getDataMemOffset(), VerboseFlag.was_set());

  if (InterpreterFlag.was_set()) {
    RftChosen = std::make_unique<dbt::NullRFT>(TheManager);
  } else {
    std::string RFTName = RFTFlag.get_value();
    if (RFTName == "net") {
      std::cerr << "NET RFT Selected\n";
      RftChosen = std::make_unique<dbt::NET>(TheManager);
    } else if (RFTName == "mret2") {
      std::cerr << "MRET2 RFT Selected\n";
      RftChosen = std::make_unique<dbt::MRET2>(TheManager);
    } else if (RFTName == "netplus") {
      std::cerr << "NETPlus RFT Selected\n";
      RftChosen = std::make_unique<dbt::NETPlus>(TheManager);
    } else if (RFTName == "lei") {
      std::cerr << "LEI rft selected\n";
      RftChosen = std::make_unique<dbt::LEI>(TheManager);
    } else if (RFTName == "MB") {
      std::cerr << "MethodBased rft selected\n";
      if (ToCompileFlag.was_set())
        RftChosen = std::make_unique<dbt::MethodBased>(TheManager, ToCompileFlag.get_value());
      else 
        RftChosen = std::make_unique<dbt::MethodBased>(TheManager);
    } else {
      std::cerr << "You should select a valid RFT!\n";
      return 1;
    }
  }

  if(HotnessFlag.was_set()) {
    std::cerr << "The Hotness Threshold is set to " << HotnessFlag.get_value() << std::endl;
    RftChosen->setHotnessThreshold(HotnessFlag.get_value());
  }

  if (RegionLimitSize.was_set()) 
    RftChosen->setRegionLimitSize(RegionLimitSize.get_value());

  std::unique_ptr<dbt::SyscallManager> SyscallM;
  SyscallM = std::make_unique<dbt::LinuxSyscallManager>();

  if (PreheatFlag.was_set()) {
    std::cerr << "Preheating... ";
    dbt::ITDInterpreter I(*SyscallM.get(), *RftChosen.get());
    I.executeAll(M);
    std::cerr << "done\n";

    std::cerr << "Cleaning VM... ";
    M.reset();
    std::cerr << "done\n";

    RftChosen = std::make_unique<dbt::PreheatRFT>(TheManager);
  }

  GlobalTimer.startClock();
  dbt::ITDInterpreter I(*SyscallM.get(), *RftChosen.get());
  std::cerr << "Starting execution:\n";
  I.executeAll(M);
  GlobalTimer.stopClock();

  GlobalTimer.printReport("Global");

  if(ReportFileFlag.was_set()) {
    ofstream report;
    report.open(ReportFileFlag.get_value());

    if(report.is_open()) {
      report << "No. Compiled Regions | " 
             << "Avg Code Size Reduction | "
             << "No. OI Instructions Compiled | " 
             << "No. LLVM compiled | "
             << "No. of Native Instructions Executed | "
             << "Total Cycles | "
             << "Conditional Branch Instructions Executed | "
             << "Conditional Branch Instructions Misses | "
             << "Total L1 I-Cache misses | "
             << "Time (s)"
             << "\n";
      report << std::dec << TheManager.getCompiledRegions() << std::endl;
      report << TheManager.getAvgOptCodeSize()/TheManager.getCompiledRegions() << std::endl;
      report << TheManager.getOICompiled() << std::endl;
      report << TheManager.getLLVMCompiled() << std::endl;
      GlobalTimer.printReport("", report);
    }

    else
      std::cerr << "Report Error: Unable to open file: \'" << ReportFileFlag.get_value() << "\' for writting.\n";
  }

  return SyscallM->getExitStatus();
}

/*
 * TODO:
 *  - Add all instructions need by printf (commit)
 *  - Correct bugs in at least 6 tests (commit)
 *  - Add new benchmarks (commit)
 *  - Make improvements on the code (commit)
 *  - Make improvements on the performance (commit)
 *  ---------------------------------------------------- Until: 24 May
*/
