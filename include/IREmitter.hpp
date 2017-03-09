#ifndef IREMITTER_HPP
#define IREMITTER_HPP

#include <OIDecoder.hpp>

#include "llvm/IR/Function.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"

#include <vector>
#include <unordered_map>
#include <memory>

#define OIInstList std::vector<uint32_t>

namespace dbt {
	class IREmitter {
	private:
		llvm::LLVMContext TheContext;
		std::unique_ptr<llvm::Module> TheModule;
		std::unordered_map<std::string, llvm::Value*> NamedValues;
		std::unique_ptr<llvm::IRBuilder<>> Builder;

		llvm::StringRef generateUniqueName(llvm::StringRef);
    llvm::Value* generateInstIR(const OIDecoder::OIInst);
  public:
		IREmitter() {
			TheModule = std::make_unique<llvm::Module>("DBT-Module", TheContext);
			Builder = std::make_unique<llvm::IRBuilder<>>(TheContext);
		};

    llvm::Function* generateRegionIR(const OIInstList);
  };
}

#endif
