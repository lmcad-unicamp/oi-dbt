#ifndef IREMITTER_HPP
#define IREMITTER_HPP

#include <OIDecoder.hpp>

#include "llvm/IR/Function.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"

// Opt
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"

#include <vector>
#include <array>
#include <unordered_map>
#include <memory>

#define OIInstList std::vector<std::array<uint32_t,2>>

namespace dbt {
	class IREmitter {
	private:
		llvm::LLVMContext TheContext;
		std::unique_ptr<llvm::Module> TheModule;
		std::unordered_map<std::string, llvm::Value*> NamedValues;
		std::unique_ptr<llvm::IRBuilder<>> Builder;
    std::unique_ptr<llvm::legacy::FunctionPassManager> TheFPM; 

    std::unordered_map<uint32_t, llvm::Value*> IRMemoryMap;
    std::unordered_map<uint32_t, llvm::BranchInst*> IRBranchMap;

    void processBranchesTargets(const OIInstList&);
    void generateInstIR(const uint32_t, const OIDecoder::OIInst);

    llvm::Value* genDataMemVecPtr(llvm::Value*, llvm::Function*);
    llvm::Value* genRegisterVecPtr(uint8_t, llvm::Function*);
    llvm::Value* genLoadRegister(uint8_t, llvm::Function*);
    llvm::Value* genStoreRegister(uint8_t, llvm::Value*, llvm::Function*);
    llvm::Value* genImm(uint32_t);
  public:
		IREmitter() {
			TheModule = std::make_unique<llvm::Module>("DBT-Module", TheContext);
			Builder = std::make_unique<llvm::IRBuilder<>>(TheContext);
      TheFPM = llvm::make_unique<llvm::legacy::FunctionPassManager>(TheModule.get());
      TheFPM->add(llvm::createInstructionCombiningPass());
      TheFPM->add(llvm::createGVNPass());
      TheFPM->add(llvm::createLICMPass());
    };

    llvm::Function* generateRegionIR(const OIInstList&, uint32_t);
  };
}

#endif
