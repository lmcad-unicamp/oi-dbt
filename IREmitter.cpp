#include <IREmitter.hpp>

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Type.h"

#include "llvm/Support/raw_ostream.h"

#include <cstdlib>

using namespace llvm;


void dbt::IREmitter::addFirstInstToMap(uint32_t GuestAddrs) {
  IRMemoryMap[GuestAddrs] = FirstInstGen;
  FirstInstGen = nullptr;
}

void dbt::IREmitter::setIfNotTheFirstInstGen(Value* Inst) {
  if (FirstInstGen == nullptr)  
    FirstInstGen = Inst;
}

Value* dbt::IREmitter::genDataMemVecPtr(Value* RawAddrs, Function* Func) {
  Value* AddrsOff = Builder->CreateSub(RawAddrs, genImm(DataMemOffset));
  Value* Addrs = Builder->CreateExactSDiv(RawAddrs, genImm(4));
  Argument *ArgDataMemPtr = &*(++Func->arg_begin()); 
  setIfNotTheFirstInstGen(AddrsOff);
  return Builder->CreateGEP(Type::getInt32Ty(TheContext), ArgDataMemPtr, Addrs);
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

void dbt::IREmitter::generateInstIR(const uint32_t GuestAddr, const dbt::OIDecoder::OIInst Inst) {
  Function* Func = Builder->GetInsertBlock()->getParent();

  switch (Inst.Type) {
    case dbt::OIDecoder::Ldi: {
        genStoreRegister(64, genImm(Inst.RT), Func);
        Value* R1 = Builder->CreateAnd(genLoadRegister(Inst.RT, Func), genImm(0xFFFFC000));
        Value* Res = Builder->CreateOr(R1, genImm(Inst.Imm & 0x3FFF));
        genStoreRegister(Inst.RT, Res, Func);
        break;
      }

    case dbt::OIDecoder::Ldihi: {
        Value* R1 = Builder->CreateAnd(genLoadRegister(genLoadRegister(64, Func), Func), genImm(0x3FFF));
        Value* Res = Builder->CreateOr(R1, genImm(Inst.Addrs << 14));
        genStoreRegister(genLoadRegister(64, Func), Res, Func);
        break;
      }

    case dbt::OIDecoder::Ori: {
        Value* Res = Builder->CreateOr(genLoadRegister(Inst.RS, Func), genImm(Inst.Imm & 0x3FFF));
        genStoreRegister(Inst.RT, Res, Func);
        break;
      }

    case dbt::OIDecoder::Mod: {
        Value* Res = Builder->CreateSRem(genLoadRegister(Inst.RS, Func), genLoadRegister(Inst.RT, Func));
        genStoreRegister(Inst.RV, Res, Func);
        break;
      }

    case dbt::OIDecoder::Shl: {
        Value* Res = Builder->CreateShl(genLoadRegister(Inst.RT, Func), genImm(Inst.RS));
        genStoreRegister(Inst.RD, Res, Func);
        break;
      }

    case dbt::OIDecoder::Shr: {
        Value* Res = Builder->CreateAShr(genLoadRegister(Inst.RT, Func), genImm(Inst.RS));
        genStoreRegister(Inst.RD, Res, Func);
        break;
      }

    case dbt::OIDecoder::Div: {
        Value* Res = Builder->CreateExactSDiv(genLoadRegister(Inst.RS, Func), genLoadRegister(Inst.RT, Func));
        genStoreRegister(Inst.RD, Res, Func);
        break;
      }

    case dbt::OIDecoder::Mul: {
        Value* Res = Builder->CreateMul(genLoadRegister(Inst.RS, Func), genLoadRegister(Inst.RT, Func));
        if (Inst.RD != 0) { 
          Value* ShiftedRes = Builder->CreateAnd(Res, genImm(0xFFFFFFFF));
          genStoreRegister(Inst.RD, ShiftedRes, Func);
        } 
        if (Inst.RV != 0) {
          Value* ShiftedRes = Builder->CreateAnd(Builder->CreateAShr(Res, 32), genImm(0xFFFFFFFF));
          genStoreRegister(Inst.RV, ShiftedRes, Func);
        }
        break;
      }

    case dbt::OIDecoder::Or: {
        Value* Res = Builder->CreateOr(genLoadRegister(Inst.RS, Func), genLoadRegister(Inst.RT, Func));
        genStoreRegister(Inst.RD, Res, Func);
        break;
      }

    case dbt::OIDecoder::And: {
        Value* Res = Builder->CreateAnd(genLoadRegister(Inst.RS, Func), genLoadRegister(Inst.RT, Func));
        genStoreRegister(Inst.RD, Res, Func);
        break;
      }

    case dbt::OIDecoder::Sub: {
        Value* Res = Builder->CreateSub(genLoadRegister(Inst.RS, Func), genLoadRegister(Inst.RT, Func));
        genStoreRegister(Inst.RD, Res, Func);
        break;
      }

    case dbt::OIDecoder::Add: {
        Value* Res = Builder->CreateAdd(genLoadRegister(Inst.RS, Func), genLoadRegister(Inst.RT, Func));
        genStoreRegister(Inst.RD, Res, Func);
        break;
      }

    case dbt::OIDecoder::Addi: {
        Value* Res = Builder->CreateAdd(genLoadRegister(Inst.RS, Func), genImm(Inst.Imm));
        genStoreRegister(Inst.RT, Res, Func);
        break;
      }

    case dbt::OIDecoder::Ldw: {
        Value* RawAddrs = Builder->CreateAdd(genLoadRegister(Inst.RS, Func), genImm(Inst.Imm));
        Value* Res = Builder->CreateLoad(genDataMemVecPtr(RawAddrs, Func));
        genStoreRegister(Inst.RT, Res, Func);
        break;
      }

    case dbt::OIDecoder::Stw: {
        Value* RawAddrs = Builder->CreateAdd(genLoadRegister(Inst.RS, Func), genImm(Inst.Imm));
        Value* Res = Builder->CreateStore(genLoadRegister(Inst.RT, Func), genDataMemVecPtr(RawAddrs, Func));
        break;
      }

    case dbt::OIDecoder::Slti: {
        Value* Res = Builder->CreateICmpSLT(genLoadRegister(Inst.RS, Func), genImm(Inst.Imm));
        Value* ResCasted = Builder->CreateIntCast(Res, Type::getInt32Ty(TheContext), true);
        genStoreRegister(Inst.RT, ResCasted, Func);
        break;
      }

    case dbt::OIDecoder::Slt: {
        Value* Res = Builder->CreateICmpSLT(genLoadRegister(Inst.RS, Func), genLoadRegister(Inst.RT, Func));
        Value* ResCasted = Builder->CreateIntCast(Res, Type::getInt32Ty(TheContext), true);
        genStoreRegister(Inst.RT, ResCasted, Func);
        break;
      }

    case dbt::OIDecoder::Sltiu: {
        Value* Res = Builder->CreateICmpSLT(genLoadRegister(Inst.RS, Func), genImm(Inst.Imm & 0x3FFF));
        Value* ResCasted = Builder->CreateIntCast(Res, Type::getInt32Ty(TheContext), true);
        genStoreRegister(Inst.RT, ResCasted, Func);
        break;
      }

    case dbt::OIDecoder::Jeqz: {
        BasicBlock* BB = BasicBlock::Create(TheContext, "", Func);
        Value* Res = Builder->CreateICmpEQ(genLoadRegister(Inst.RS, Func), genImm(0));
        BranchInst* Br = Builder->CreateCondBr(Res, BB, BB);
        Builder->SetInsertPoint(BB);
        IRBranchMap[GuestAddr] = Br;
        break;
      }

    case dbt::OIDecoder::Jeq: {
        BasicBlock* BB = BasicBlock::Create(TheContext, "", Func);
        Value* Res = Builder->CreateICmpEQ(genLoadRegister(Inst.RT, Func), genLoadRegister(Inst.RS, Func));
        BranchInst* Br = Builder->CreateCondBr(Res, BB, BB);
        Builder->SetInsertPoint(BB);
        IRBranchMap[GuestAddr] = Br;
        break;
      }

    case dbt::OIDecoder::Jne: {
        BasicBlock* BB = BasicBlock::Create(TheContext, "", Func);
        Value* Res = Builder->CreateICmpNE(genLoadRegister(Inst.RT, Func), genLoadRegister(Inst.RS, Func));
        BranchInst* Br = Builder->CreateCondBr(Res, BB, BB);
        Builder->SetInsertPoint(BB);
        IRBranchMap[GuestAddr] = Br;
        break;
      }

    case dbt::OIDecoder::Jump: { 
        BasicBlock* BB = BasicBlock::Create(TheContext, "", Func);
        BranchInst* Br = Builder->CreateBr(BB);
        Builder->SetInsertPoint(BB);
        setIfNotTheFirstInstGen(Br);
        IRBranchMap[GuestAddr] = Br;
        break;
      }

    case dbt::OIDecoder::Jumpr: { // TODO: improve indirect Jump
        Builder->CreateRet(genLoadRegister(Inst.RT, Func));
        BasicBlock* BB = BasicBlock::Create(TheContext, "", Func);
        Builder->SetInsertPoint(BB);
        break;
      }

    case dbt::OIDecoder::Call: {
        BasicBlock* BB = BasicBlock::Create(TheContext, "", Func);
        BranchInst* Br = Builder->CreateBr(BB);
        Builder->SetInsertPoint(BB);
        setIfNotTheFirstInstGen(Br);
        IRBranchMap[GuestAddr] = Br;
        break;
      }
  }

  addFirstInstToMap(GuestAddr);
}

void dbt::IREmitter::updateBranchTarget(uint32_t GuestAddr, std::array<uint32_t, 2> Tgts) {
  Function* F = Builder->GetInsertBlock()->getParent();
  for (int i = 0; i < 2; i++) {
    uint32_t AddrTarget = Tgts[i];

    if (AddrTarget == 0) continue;

    BasicBlock *BBTarget;
    if (IRMemoryMap.count(AddrTarget) != 0) {
      auto TargetInst = cast<Instruction>(IRMemoryMap[AddrTarget]);
      BasicBlock *Current = TargetInst->getParent();

      if (Current->getFirstNonPHI() == TargetInst) 
        BBTarget = Current;
      else
        BBTarget = Current->splitBasicBlock(TargetInst);
    } else {
      BBTarget = BasicBlock::Create(TheContext, "", F);
      Builder->SetInsertPoint(BBTarget);
      Builder->CreateRet(genImm(AddrTarget));
    }
    IRBranchMap[GuestAddr]->setSuccessor(i, BBTarget);
  }
}

void dbt::IREmitter::cleanCFG() {
  Function* F = Builder->GetInsertBlock()->getParent();
  BasicBlock* Trash = nullptr;
  for (auto& BB : *F) 
    if (BB.size() == 0)
      Trash = &BB;

  if (Trash)
    Trash->eraseFromParent();
}

void dbt::IREmitter::processBranchesTargets(const OIInstList& OIRegion) {
  for (auto Pair : OIRegion) {
    OIDecoder::OIInst Inst = OIDecoder::decode(Pair[1]);
    uint32_t GuestAddr = Pair[0];

    switch (Inst.Type) {
      case dbt::OIDecoder::Jne: 
      case dbt::OIDecoder::Jeqz: 
      case dbt::OIDecoder::Jeq: 
        {
          updateBranchTarget(GuestAddr, {(GuestAddr + (Inst.Imm << 2)) + 4, GuestAddr + 4});
          break;
        }
      case dbt::OIDecoder::Jump: 
        {
          updateBranchTarget(GuestAddr, {(GuestAddr & 0xF0000000) | (Inst.Addrs << 2), 0});
          break;
        }
    }
  }
  
  cleanCFG();
}

Module* dbt::IREmitter::generateRegionIR(uint32_t EntryAddress, const OIInstList& OIRegion, uint32_t MemOffset) {
  Module* TheModule = new Module("", TheContext);
  IRMemoryMap.clear();
  IRBranchMap.clear();

  DataMemOffset = MemOffset;

  //int32_t execRegion(int32_t* IntRegisters, int32_t* DataMemory);
  std::array<Type*, 2> ArgsType = {Type::getInt32PtrTy(TheContext), Type::getInt32PtrTy(TheContext)};
  FunctionType *FT = FunctionType::get(Type::getInt32Ty(TheContext), ArgsType, false);
  Function *F = Function::Create(FT, Function::ExternalLinkage, "r"+std::to_string(EntryAddress), TheModule);
  F->addAttribute(1, Attribute::NoAlias);
  F->addAttribute(2, Attribute::NoAlias);
  F->addAttribute(1, Attribute::NoCapture);
  F->addAttribute(2, Attribute::NoCapture);

  // Entry block to function must not have predecessors!
  BasicBlock *Entry = BasicBlock::Create(TheContext, "entry", F);
  BasicBlock *BB = BasicBlock::Create(TheContext, "", F);

  Builder->SetInsertPoint(Entry);
  Builder->CreateBr(BB);
  Builder->SetInsertPoint(BB);

  for (auto Pair : OIRegion) {
    OIDecoder::OIInst Inst = OIDecoder::decode(Pair[1]);
    generateInstIR(Pair[0], Inst);
  }

  processBranchesTargets(OIRegion);

  for (auto& BB : *F) {
    if (BB.getTerminator() == nullptr) {
      Builder->SetInsertPoint(&BB);
      Builder->CreateRet(genImm(OIRegion.back()[0]+4));
    }
  }

  return TheModule;
}
