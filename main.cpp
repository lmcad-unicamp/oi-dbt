#include <arglib/arglib.hpp>
#include <interpreter.hpp>
#include <RFT.hpp>
#include <manager.hpp>
#include <syscall.hpp>

#include <iostream>
#include <memory>

clarg::argString RFTFlag("-rft", "Region Formation Technique (net)", "net");
clarg::argBool   InterpreterFlag("-interpret",  "Only interpret.");
clarg::argString BinaryFlag("-bin",  "path to the binary which will should be emulated.", "");
clarg::argBool   HelpFlag("-h",  "display the help message");

void usage(char* PrgName) {
  cout << "Version: 0.0.1 (03-01-2017)\n\n";

  cout << "Usage: " << PrgName << 
    " [-rft net] [-interpreter] -bin PathToBinary\n\n";

  cout << "DESCRIPTION:\n";
  cout << "This program implements the OpenISA DBT (Dynamic Binary Translator)\n" <<
    "Institute of Computing, 2017.\n\n";

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

int main(int argc, char** argv) {
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


  dbt::Machine M;

  int loadStatus = M.loadELF(BinaryFlag.get_value());

  if (!loadStatus) {
    std::cout << "Can't find or process ELF file " << argv[1] << std::endl;
    return 2;
  }

  dbt::Manager TheManager(1, dbt::Manager::OptPolitic::Normal, M.getDataMemOffset());

  std::unique_ptr<dbt::RFT> RftChosen;

  if (InterpreterFlag.was_set()) {
    RftChosen = std::make_unique<dbt::NullRFT>(TheManager);
  } else {
    std::string RFTName = RFTFlag.get_value();
    if (RFTName == "net") {
      std::cout << "NET RFT Selected\n";
      RftChosen = std::make_unique<dbt::NET>(TheManager);
    } else if (RFTName == "mret2") {
      std::cout << "MRET2 RFT Selected\n";
      RftChosen = std::make_unique<dbt::MRET2>(TheManager);
    } else if (RFTName == "netplus") {
      std::cout << "NETPlus RFT Selected\n";
      RftChosen = std::make_unique<dbt::NETPlus>(TheManager);
    } else {
      std::cerr << "You should select a valid RFT!\n";
      return 1;
    }
  }

  std::unique_ptr<dbt::SyscallManager> SyscallM;
  SyscallM = std::make_unique<dbt::LinuxSyscallManager>();

  dbt::ITDInterpreter I(*SyscallM.get(), *RftChosen.get());
  I.executeAll(M);

  //RftChosen->printRegions(); <- Not thread safe

  return SyscallM->getExitStatus();
}

/*
 * TODO:
 *  - Implement and test all instructions of the interpreter into the IREmitter
 *  - Make corrections on the region transition 
 *  - Flags para melhorar controle: -only {interpret|rft|compilation}
 */
