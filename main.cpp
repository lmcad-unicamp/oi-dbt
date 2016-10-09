#include <interpreter.hpp>
#include <RFT.hpp>

#include <iostream>

int main(int argc, char** argv) {
  if ( argc != 2 ) {
    std::cout << "Usage: oi-dbt <elf_file>" << std::endl;
    return 1;
  }

  dbt::Machine M;
  int loadStatus = M.loadELF(std::string(argv[1]));

  dbt::NET Net;
  Net.installRFT(M);

  if (!loadStatus) {
    std::cout << "Can't find or process ELF file " << argv[1] << std::endl;
    return 2;
  }

  dbt::ITDInterpreter I;
  I.executeAll(M);

  Net.printRegions(M);

  return 0;
}
