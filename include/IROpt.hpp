#ifndef IROpt_HPP
#define IROpt_HPP

#include "llvm/IR/Module.h"

namespace dbt {
	class IROpt {
  public:
    IROpt() {}; 

    enum OptLevel { Basic, Soft, Medium, Hard };

    void optimizeIRFunction(llvm::Module*, OptLevel);
  };
}

#endif
