#ifndef IREMITTER_HPP
#define IREMITTER_HPP

#include <OIDecoder.hpp>

#include "llvm/IR/Function.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Type.h"

#include <vector>
#include <array>
#include <unordered_map>
#include <memory>

#define OIInstList std::vector<std::array<uint32_t,2>>

namespace dbt {
  class IREmitter {
  private:
    enum RegType {
      Int, Float, Double
    };

		llvm::LLVMContext TheContext;
		std::unordered_map<std::string, llvm::Value*> NamedValues;
		std::unique_ptr<llvm::IRBuilder<>> Builder;

    uint32_t DataMemOffset;
    uint32_t CurrentEntryAddrs;

    llvm::Value* FirstInstGen;
    void addFirstInstToMap(uint32_t);
    void setIfNotTheFirstInstGen(llvm::Value*);

    std::unordered_map<uint32_t, llvm::Value*> IRMemoryMap;
    std::unordered_map<uint32_t, llvm::BranchInst*> IRBranchMap;

    std::unordered_map<uint32_t, std::vector<uint32_t>> DirectTransitions; // FIXME: THIS SHOULDN'T BE IN THIS CLASS!

    void cleanCFG();
    void updateBranchTarget(uint32_t, std::array<uint32_t, 2>);
    void processBranchesTargets(const OIInstList&);
    void generateInstIR(const uint32_t, const OIDecoder::OIInst);

    llvm::Value* genDataVecPtr(llvm::Value*, llvm::Function*, llvm::Type*, unsigned);
    llvm::Value* genDataMemVecPtr(llvm::Value*, llvm::Function*);
    llvm::Value* genDataByteVecPtr(llvm::Value*, llvm::Function*);
    llvm::Value* genDataHalfVecPtr(llvm::Value*, llvm::Function*);
    llvm::Value* genDataWordVecPtr(llvm::Value*, llvm::Function*);
    llvm::Value* genRegisterVecPtr(uint16_t, llvm::Function*, RegType);
    llvm::Value* genRegisterVecPtr(llvm::Value*, llvm::Function*, RegType);

    llvm::Value* genImm(uint32_t);

    llvm::Value* genLoadRegister(uint16_t, llvm::Function*, RegType Type = RegType::Int);
    llvm::Value* genLoadRegister(llvm::Value*, llvm::Function*, RegType Type = RegType::Int);
    llvm::Value* genStoreRegister(uint16_t, llvm::Value*, llvm::Function*, RegType Type = RegType::Int);
    llvm::Value* genStoreRegister(llvm::Value*, llvm::Value*, llvm::Function*, RegType Type = RegType::Int);

    llvm::Value* genLogicalOr(llvm::Value*, llvm::Value*, llvm::Function*);
    llvm::Value* genLogicalAnd(llvm::Value*, llvm::Value*, llvm::Function*);

    void insertDirectExit(uint32_t);
  public:
		IREmitter() {
			Builder = std::make_unique<llvm::IRBuilder<>>(TheContext);
    };

    std::vector<uint32_t> getDirectTransitions(uint32_t Addrs) { // Mother of GOD TODO: remove it from here!
      return DirectTransitions[Addrs];
    }

    llvm::Module* generateRegionIR(uint32_t, const OIInstList&, uint32_t);
  };
}

#endif
