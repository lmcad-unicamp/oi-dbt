#include <IREmitter.hpp>
#include <OIPrinter.hpp>

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Type.h"

#include "llvm/IR/CFG.h"

#include "llvm/Support/raw_ostream.h"

#include <cstdlib>

using namespace llvm;

void dbt::IREmitter::insertDirectExit(uint32_t ExitAddrs) {
  Builder->CreateRet(genImm(ExitAddrs));
  if (DirectTransitions.count(ExitAddrs) == 0)
    DirectTransitions[ExitAddrs];
  DirectTransitions[ExitAddrs].push_back(CurrentEntryAddrs);
}

void dbt::IREmitter::addFirstInstToMap(uint32_t GuestAddrs) {
  IRMemoryMap[GuestAddrs] = FirstInstGen;
  FirstInstGen = nullptr;
}

void dbt::IREmitter::setIfNotTheFirstInstGen(Value* Inst) {
  if (FirstInstGen == nullptr)  
    FirstInstGen = Inst;
}

Value* dbt::IREmitter::genDataVecPtr(Value* RawAddrs, Function* Func, Type* IType, unsigned ByteSize) {
  Value* AddrsOff = Builder->CreateSub(RawAddrs, genImm(DataMemOffset));
  Value* Addrs = Builder->CreateExactSDiv(AddrsOff, genImm(ByteSize));
  Argument *ArgDataMemPtr = &*(++Func->arg_begin()); 
  Value* CastedPtr = Builder->CreatePointerCast(ArgDataMemPtr, Type::getIntNPtrTy(TheContext, ByteSize*8));
  setIfNotTheFirstInstGen(AddrsOff);
  return Builder->CreateGEP(IType, CastedPtr, Addrs);
}

Value* dbt::IREmitter::genDataByteVecPtr(Value* RawAddrs, Function* Func) {
  return genDataVecPtr(RawAddrs, Func, Type::getInt8Ty(TheContext), 1);
}

Value* dbt::IREmitter::genDataHalfVecPtr(Value* RawAddrs, Function* Func) {
  return genDataVecPtr(RawAddrs, Func, Type::getInt16Ty(TheContext), 2);
}

Value* dbt::IREmitter::genDataWordVecPtr(Value* RawAddrs, Function* Func) {
  return genDataVecPtr(RawAddrs, Func, Type::getInt32Ty(TheContext), 4);
}

Value* dbt::IREmitter::genRegisterVecPtr(Value* RegNum, Function* Func) {
  Argument *ArgIntRegPtr = &*Func->arg_begin(); 
  Value* GEP = Builder->CreateGEP(ArgIntRegPtr, RegNum);
  setIfNotTheFirstInstGen(GEP);
  return GEP;
}

Value* dbt::IREmitter::genRegisterVecPtr(uint8_t RegNum, Function* Func) {
  Argument *ArgIntRegPtr = &*Func->arg_begin(); 
  Value* GEP = Builder->CreateGEP(ArgIntRegPtr, ConstantInt::get(Type::getInt32Ty(TheContext), RegNum));
  setIfNotTheFirstInstGen(GEP);
  return GEP;
}

Value* dbt::IREmitter::genLoadRegister(Value* R, Function* Func) {
  Value* Ptr = genRegisterVecPtr(R, Func);
  return Builder->CreateLoad(Ptr);
}

Value* dbt::IREmitter::genLoadRegister(uint8_t RegNum, Function* Func) {
  if (RegNum == 0)
    return genImm(0); 

  Value* Ptr = genRegisterVecPtr(RegNum, Func);
  return Builder->CreateLoad(Ptr);
}

Value* dbt::IREmitter::genStoreRegister(Value* R, Value* V, Function* Func) {
  Value* Ptr = genRegisterVecPtr(R, Func);
  return Builder->CreateStore(V, Ptr);
}

Value* dbt::IREmitter::genStoreRegister(uint8_t RegNum, Value* V, Function* Func) {
  Value* Ptr = genRegisterVecPtr(RegNum, Func);
  return Builder->CreateStore(V, Ptr);
}

Value* dbt::IREmitter::genImm(uint32_t Imm) {
  return ConstantInt::get(Type::getInt32Ty(TheContext), Imm);
}

Value* dbt::IREmitter::genLogicalOr(Value* Lhs, Value* Rhs, Function* Func) {
  BasicBlock* TBB = BasicBlock::Create(TheContext, "Lor.end", Func);
  BasicBlock* FBB = BasicBlock::Create(TheContext, "Lor.RHS", Func);
  BasicBlock* LastBB = Builder->GetInsertBlock();

  Value* Res1 = Builder->CreateICmpNE(Lhs, genImm(0));
  BranchInst* Br = Builder->CreateCondBr(Res1, TBB, FBB);

  Builder->SetInsertPoint(FBB);
  Value* Res2 = Builder->CreateICmpNE(Rhs, genImm(0));
  Builder->CreateBr(TBB);

  Builder->SetInsertPoint(TBB);
  PHINode* Node = Builder->CreatePHI(Res1->getType(), 2);
  Node->addIncoming(Res1, LastBB);
  Node->addIncoming(Res2, FBB);

  setIfNotTheFirstInstGen(Res1);
  return Node;
}

Value* dbt::IREmitter::genLogicalAnd(Value* Lhs, Value* Rhs, Function* Func) {
  BasicBlock* TBB = BasicBlock::Create(TheContext, "Land.RHS", Func);
  BasicBlock* FBB = BasicBlock::Create(TheContext, "Land.end", Func);
  BasicBlock* LastBB = Builder->GetInsertBlock();

  Value* Res1 = Builder->CreateICmpNE(Lhs, genImm(0));
  BranchInst* Br = Builder->CreateCondBr(Res1, TBB, FBB);

  Builder->SetInsertPoint(TBB);
  Value* Res2 = Builder->CreateICmpNE(Rhs, genImm(0));
  Builder->CreateBr(FBB);

  Builder->SetInsertPoint(FBB);
  PHINode* Node = Builder->CreatePHI(Res1->getType(), 2);
  Node->addIncoming(Res1, LastBB);
  Node->addIncoming(Res2, TBB);

  setIfNotTheFirstInstGen(Res1);
  return Node;
}

void dbt::IREmitter::cleanCFG() {
  Function* F = Builder->GetInsertBlock()->getParent();
  std::vector<BasicBlock*> TrashBlocks;
  for (auto& BB : *F) 
    if (BB.size() == 0 || (pred_empty(&BB) && BB.getName() != "entry")) 
      TrashBlocks.push_back(&BB);

  for (auto& BB : TrashBlocks)
    BB->eraseFromParent();
}

