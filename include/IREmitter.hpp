#ifndef IREMITTER_HPP
#define IREMITTER_HPP

#include <OIDecoder.hpp>

#include "llvm/IR/Function.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"

#include <vector>
#include <array>
#include <unordered_map>
#include <memory>

#define OIInstList std::vector<std::array<uint32_t,2>>

namespace dbt {
	class IREmitter {
	private:
		llvm::LLVMContext TheContext;
		std::unordered_map<std::string, llvm::Value*> NamedValues;
		std::unique_ptr<llvm::IRBuilder<>> Builder;

    uint32_t DataMemOffset;

    llvm::Value* FirstInstGen;
    void addFirstInstToMap(uint32_t);
    void setIfNotTheFirstInstGen(llvm::Value*);

    std::unordered_map<uint32_t, llvm::Value*> IRMemoryMap;
    std::unordered_map<uint32_t, llvm::BranchInst*> IRBranchMap;

    void cleanCFG();
    void updateBranchTarget(uint32_t, std::array<uint32_t, 2>);
    void processBranchesTargets(const OIInstList&);
    void generateInstIR(const uint32_t, const OIDecoder::OIInst);

    llvm::Value* genDataMemVecPtr(llvm::Value*, llvm::Function*);
    llvm::Value* genDataByteVecPtr(llvm::Value*, llvm::Function*);
    llvm::Value* genRegisterVecPtr(uint8_t, llvm::Function*);
    llvm::Value* genRegisterVecPtr(llvm::Value*, llvm::Function*);
    llvm::Value* genLoadRegister(uint8_t, llvm::Function*);
    llvm::Value* genLoadRegister(llvm::Value*, llvm::Function*);
    llvm::Value* genStoreRegister(uint8_t, llvm::Value*, llvm::Function*);
    llvm::Value* genStoreRegister(llvm::Value*, llvm::Value*, llvm::Function*);
    llvm::Value* genImm(uint32_t);

    llvm::Value* genLogicalOr(llvm::Value*, llvm::Value*, llvm::Function*);
    llvm::Value* genLogicalAnd(llvm::Value*, llvm::Value*, llvm::Function*);
  public:
		IREmitter() {
			Builder = std::make_unique<llvm::IRBuilder<>>(TheContext);
    };

    llvm::Module* generateRegionIR(uint32_t, const OIInstList&, uint32_t);
  };
}

#endif
