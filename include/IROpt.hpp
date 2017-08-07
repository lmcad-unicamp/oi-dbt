#ifndef IROpt_HPP
#define IROpt_HPP

#include "llvm/IR/Module.h"
#include "llvm/IR/LegacyPassManager.h"

namespace dbt {
	class IROpt {
    std::unique_ptr<llvm::legacy::FunctionPassManager> BasicPM;
  public:
    IROpt() {}; 

    enum OptLevel { Basic, Soft, Medium, Hard };

    void optimizeIRFunction(llvm::Module*, OptLevel);
  };
}

#endif
