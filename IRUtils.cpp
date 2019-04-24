#include <IREmitter.hpp>
#include <OIPrinter.hpp>

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Constants.h"

#include "llvm/IR/CFG.h"

#include "llvm/Support/raw_ostream.h"

#include <cstdlib>

using namespace llvm;

void dbt::IREmitter::emmitExit(Function* Func) {
  auto IP = Builder->saveIP();
  Builder->SetInsertPoint(RegionExit);
  for (auto P : VolatileRegisters) {
    if (VolatileRegisterModified.count(P.first) != 0) 
      Builder->CreateStore(Builder->CreateLoad(P.second), Builder->CreateGEP(&*Func->arg_begin(), genImm(P.first)));
  }
  Builder->CreateRet(Builder->CreateLoad(ReturnAddrs));
  Builder->restoreIP(IP);
}

llvm::Value* dbt::IREmitter::insertDirectExit(llvm::Value* ExitAddrs) {
  auto First = Builder->CreateStore(ExitAddrs, ReturnAddrs);
  Builder->CreateBr(RegionExit);
  return First;
}

void dbt::IREmitter::addFirstInstToMap(uint32_t GuestAddrs) {
  IRMemoryMap[GuestAddrs] = FirstInstGen;
  FirstInstGen = nullptr;
}

void dbt::IREmitter::setIfNotTheFirstInstGen(Value *Inst) {
  if (FirstInstGen == nullptr && dyn_cast<Instruction>(Inst))
    FirstInstGen = Inst;
}

Value *dbt::IREmitter::genDataVecPtr(Value *RawAddrs, Function *Func, Type *IType, unsigned ByteSize) {
  Value *AddrsOff = Builder->CreateSub(RawAddrs, genImm(DataMemOffset));
  Argument *ArgDataMemPtr = &*(Func->arg_begin()+1);
  Value *MPtr8    = Builder->CreatePointerCast(ArgDataMemPtr, Type::getInt8PtrTy(TheContext));
  Value *GEPMPtr8 = Builder->CreateGEP(MPtr8, AddrsOff);
  Value *CastedPtr = Builder->CreatePointerCast(GEPMPtr8, Type::getIntNPtrTy(TheContext, ByteSize * 8));
  setIfNotTheFirstInstGen(AddrsOff);
  return CastedPtr;
}

Value *dbt::IREmitter::genDataByteVecPtr(Value *RawAddrs, Function *Func) {
  return genDataVecPtr(RawAddrs, Func, Type::getInt8Ty(TheContext), 1);
}

Value *dbt::IREmitter::genDataHalfVecPtr(Value *RawAddrs, Function *Func) {
  return genDataVecPtr(RawAddrs, Func, Type::getInt16Ty(TheContext), 2);
}

Value *dbt::IREmitter::genDataWordVecPtr(Value *RawAddrs, Function *Func) {
  return genDataVecPtr(RawAddrs, Func, Type::getInt32Ty(TheContext), 4);
}

Value *dbt::IREmitter::genRegisterVecPtr(Value *RegNum, Function *Func, RegType Type) {
  Argument *ArgIntRegPtr = &*Func->arg_begin();

  Value* CastedPtr = ArgIntRegPtr;
  if (Type == RegType::Float) { 
    CastedPtr = Builder->CreatePointerCast(ArgIntRegPtr, Type::getFloatPtrTy(TheContext));
    setIfNotTheFirstInstGen(CastedPtr);
  } else if (Type == RegType::Double) {
    CastedPtr = Builder->CreatePointerCast(ArgIntRegPtr, Type::getDoublePtrTy(TheContext));
    setIfNotTheFirstInstGen(CastedPtr);
  } else if (Type == RegType::Int64) {
    CastedPtr = Builder->CreatePointerCast(ArgIntRegPtr, Type::getInt64PtrTy(TheContext));
    setIfNotTheFirstInstGen(CastedPtr);
  }

  Value *GEP = Builder->CreateGEP(CastedPtr, RegNum);
  setIfNotTheFirstInstGen(GEP);
  return GEP;
}

Value *dbt::IREmitter::genRegisterVecPtr(uint16_t RegNum, Function *Func, RegType Type) {

  return genRegisterVecPtr(ConstantInt::get(Type::getInt32Ty(TheContext), RegNum), Func, Type);
}

/********************************** Register Bank Interface *****************************************/ 

Value *dbt::IREmitter::genLoadRegister(uint16_t RegNum, Function *Func, RegType Type) {

  auto BB = Builder->GetInsertBlock();
  uint16_t Right = RegNum;
  if (Type == RegType::Float) 
    Right += 66;  
  else if (Type == RegType::Double || Type == RegType::Int64) 
    Right += 65;  

  if (Right == 0)
    return genImm(0);

  Value *Ptr = nullptr;   // Volatile Registers
  if (Type == RegType::Int) {
    if ((RegNum >= 8 && RegNum <= 15) || (RegNum == 24) || (RegNum == 25) || (RegNum >= 32 && RegNum <= 63)) {
      if (VolatileRegisters.count(RegNum) == 0) {
        auto IP = Builder->saveIP();
        Builder->SetInsertPoint(RegionEntry);
        VolatileRegisters[RegNum] = Builder->CreateAlloca(Type::getInt32Ty(TheContext));
        Builder->CreateStore(Builder->CreateLoad(Builder->CreateGEP(&*Func->arg_begin(), genImm(RegNum))), VolatileRegisters[RegNum]);
        Builder->restoreIP(IP);
      }
      Ptr = VolatileRegisters[RegNum];
    } 
  }

  if (!Ptr) Ptr = genRegisterVecPtr(Right, Func, Type);

  Value *LD  = Builder->CreateLoad(Ptr);
  setIfNotTheFirstInstGen(LD);
  return LD;
}

Value *dbt::IREmitter::genStoreRegister(uint16_t RegNum, Value *V, Function *Func, RegType Type) {
  auto BB = Builder->GetInsertBlock();
  uint16_t Right = RegNum;
  if (Type == RegType::Float) 
    Right += 66;  
  else if (Type == RegType::Double || Type == RegType::Int64) 
    Right += 65;  

  Value *Ptr = nullptr;   // Volatile Registers
  if (Type == RegType::Int) {
    if ((RegNum >= 8 && RegNum <= 15) || (RegNum == 24) || (RegNum == 25) || (RegNum >= 32 && RegNum <= 63)) {
      VolatileRegisterModified[RegNum] = true;
      if (VolatileRegisters.count(RegNum) == 0) {
        auto IP = Builder->saveIP();
        Builder->SetInsertPoint(RegionEntry);
        VolatileRegisters[RegNum] = Builder->CreateAlloca(Type::getInt32Ty(TheContext));
        Builder->restoreIP(IP);
      }
      Ptr = VolatileRegisters[RegNum];
    } 
  }

  if (!Ptr) Ptr = genRegisterVecPtr(Right, Func, Type);

  Value *ST  = Builder->CreateStore(V, Ptr);
  setIfNotTheFirstInstGen(ST);
  return ST;
}

/********************************************************************************************/ 

Value *dbt::IREmitter::genImm(uint32_t Imm) {
  return ConstantInt::get(Type::getInt32Ty(TheContext), Imm);
}

Value *dbt::IREmitter::genLogicalOr(Value *Lhs, Value *Rhs, Function *Func) {
  BasicBlock *TBB = BasicBlock::Create(TheContext, "Lor.end", Func);
  BasicBlock *FBB = BasicBlock::Create(TheContext, "Lor.RHS", Func);
  BasicBlock *LastBB = Builder->GetInsertBlock();

  Value *Res1 = Builder->CreateICmpNE(Lhs, Builder->getInt1(false));
  Builder->CreateCondBr(Res1, TBB, FBB);

  Builder->SetInsertPoint(FBB);
  Value *Res2 = Builder->CreateICmpNE(Rhs, genImm(0));
  Builder->CreateBr(TBB);

  Builder->SetInsertPoint(TBB);
  PHINode *Node = Builder->CreatePHI(Res1->getType(), 2);
  Node->addIncoming(Res1, LastBB);
  Node->addIncoming(Res2, FBB);

  setIfNotTheFirstInstGen(Res1);
  return Node;
}

Value *dbt::IREmitter::genLogicalAnd(Value *Lhs, Value *Rhs, Function *Func) {
  BasicBlock *TBB = BasicBlock::Create(TheContext, "Land.RHS", Func);
  BasicBlock *FBB = BasicBlock::Create(TheContext, "Land.end", Func);
  BasicBlock *LastBB = Builder->GetInsertBlock();

  Value *Res1 = Builder->CreateICmpNE(Lhs, genImm(0));
  Builder->CreateCondBr(Res1, TBB, FBB);

  Builder->SetInsertPoint(TBB);
  Value *Res2 = Builder->CreateICmpNE(Rhs, Builder->getInt1(false));
  Builder->CreateBr(FBB);

  Builder->SetInsertPoint(FBB);
  PHINode *Node = Builder->CreatePHI(Res1->getType(), 2);
  Node->addIncoming(Res1, LastBB);
  Node->addIncoming(Res2, TBB);

  setIfNotTheFirstInstGen(Res1);
  return Node;
}
