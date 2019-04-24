#ifndef IROpt_HPP
#define IROpt_HPP

#include "llvm/IR/Module.h"
#include "llvm/IR/LegacyPassManager.h"

namespace dbt {
	class IROpt {
    std::unique_ptr<llvm::legacy::FunctionPassManager> BasicPM;

    void populateFuncPassManager(llvm::legacy::FunctionPassManager*, std::vector<std::string>);
  public:
    IROpt() {}; 

    enum OptLevel { Basic, Soft, Medium, Hard, Custom };

    void optimizeIRFunction(llvm::Module*, OptLevel, uint32_t);
    void customOptimizeIRFunction(llvm::Module*, std::vector<std::string>);
  };
}

#endif
