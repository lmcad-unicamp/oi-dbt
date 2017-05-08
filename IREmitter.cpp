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

void dbt::IREmitter::addFirstInstToMap(uint32_t GuestAddrs) {
  IRMemoryMap[GuestAddrs] = FirstInstGen;
  FirstInstGen = nullptr;
}

void dbt::IREmitter::setIfNotTheFirstInstGen(Value* Inst) {
  if (FirstInstGen == nullptr)  
    FirstInstGen = Inst;
}

Value* dbt::IREmitter::genDataByteVecPtr(Value* RawAddrs, Function* Func) {
  Value* AddrsOff = Builder->CreateSub(RawAddrs, genImm(DataMemOffset));
  Argument *ArgDataMemPtr = &*(++Func->arg_begin()); 
  Value* CastedPtr = Builder->CreatePointerCast(ArgDataMemPtr, Type::getInt8PtrTy(TheContext));
  setIfNotTheFirstInstGen(AddrsOff);
  return Builder->CreateGEP(Type::getInt8Ty(TheContext), CastedPtr, AddrsOff);
}

Value* dbt::IREmitter::genDataMemVecPtr(Value* RawAddrs, Function* Func) {
  Value* AddrsOff = Builder->CreateSub(RawAddrs, genImm(DataMemOffset));
  Value* Addrs = Builder->CreateExactSDiv(AddrsOff, genImm(4));
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

void dbt::IREmitter::generateInstIR(const uint32_t GuestAddr, const dbt::OIDecoder::OIInst Inst) {
  Function* Func = Builder->GetInsertBlock()->getParent();

  switch (Inst.Type) {
    case dbt::OIDecoder::Nop: {
        Value* Res = Builder->CreateOr(genImm(0), genImm(0)); 
        setIfNotTheFirstInstGen(Res);
        break;
      }

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

    case dbt::OIDecoder::Xori: {
        Value* Res = Builder->CreateXor(genLoadRegister(Inst.RS, Func), genImm(Inst.Imm & 0x3FFF));
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
        Value* Res = Builder->CreateLShr(genLoadRegister(Inst.RT, Func), genImm(Inst.RS));
        genStoreRegister(Inst.RD, Res, Func);
        break;
      }

    case dbt::OIDecoder::Asr: {
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
          Value* ShiftedRes = Builder->CreateAnd(Builder->CreateLShr(Res, 32), genImm(0xFFFFFFFF));
          genStoreRegister(Inst.RV, ShiftedRes, Func);
        }
        break;
      }

    case dbt::OIDecoder::Mulu: {
        Value* URS = Builder->CreateIntCast(genLoadRegister(Inst.RS, Func), Type::getInt32Ty(TheContext), false);
        Value* URT = Builder->CreateIntCast(genLoadRegister(Inst.RT, Func), Type::getInt32Ty(TheContext), false);
        Value* Res = Builder->CreateNUWMul(URS, URT);
        if (Inst.RD != 0) { 
          Value* UShiftedRes = Builder->CreateAnd(Res, genImm(0xFFFFFFFF));
          Value* ShiftedRes = Builder->CreateIntCast(UShiftedRes, Type::getInt32Ty(TheContext), true);
          genStoreRegister(Inst.RD, ShiftedRes, Func);
        } 
        if (Inst.RV != 0) {
          Value* UShiftedRes = Builder->CreateAnd(Builder->CreateLShr(Res, 32), genImm(0xFFFFFFFF));
          Value* ShiftedRes = Builder->CreateIntCast(UShiftedRes, Type::getInt32Ty(TheContext), true);
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

    case dbt::OIDecoder::Andi: {
        Value* Res = Builder->CreateAnd(genLoadRegister(Inst.RS, Func), genImm(Inst.Imm & 0x3FFF));
        genStoreRegister(Inst.RT, Res, Func);
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

    case dbt::OIDecoder::Ldb: {
        Value* RawAddrs = Builder->CreateAdd(genLoadRegister(Inst.RS, Func), genImm(Inst.Imm));
        Value* Res = Builder->CreateLoad(genDataByteVecPtr(RawAddrs, Func));
        genStoreRegister(Inst.RT, Builder->CreateIntCast(Res, Type::getInt8Ty(TheContext), true), Func);
        break;
      }

    case dbt::OIDecoder::Ldbu: {
        Value* RawAddrs = Builder->CreateAdd(genLoadRegister(Inst.RS, Func), genImm(Inst.Imm));
        Value* Res = Builder->CreateLoad(genDataByteVecPtr(RawAddrs, Func));
        genStoreRegister(Inst.RT, Builder->CreateIntCast(Res, Type::getInt8Ty(TheContext), false), Func);
        break;
      }

    case dbt::OIDecoder::Stw: {
        Value* RawAddrs = Builder->CreateAdd(genLoadRegister(Inst.RS, Func), genImm(Inst.Imm));
        Builder->CreateStore(genLoadRegister(Inst.RT, Func), genDataMemVecPtr(RawAddrs, Func));
        break;
      }

    case dbt::OIDecoder::Stb: {
        Value* RawAddrs = Builder->CreateAdd(genLoadRegister(Inst.RS, Func), genImm(Inst.Imm));
        Value* UChar    = Builder->CreateIntCast(genLoadRegister(Inst.RT, Func), Type::getInt8Ty(TheContext), false);
        Builder->CreateStore(UChar, genDataByteVecPtr(RawAddrs, Func));
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
        genStoreRegister(Inst.RD, ResCasted, Func);
        break;
      }

    case dbt::OIDecoder::Sltiu: {
        Value* Res = Builder->CreateICmpULT(genLoadRegister(Inst.RS, Func), genImm(Inst.Imm & 0x3FFF));
        Value* ResCasted = Builder->CreateIntCast(Res, Type::getInt32Ty(TheContext), true);
        genStoreRegister(Inst.RT, ResCasted, Func);
        break;
      }

    case dbt::OIDecoder::Sltu: {
        Value* Res = Builder->CreateICmpULT(genLoadRegister(Inst.RS, Func), genLoadRegister(Inst.RT, Func));
        Value* ResCasted = Builder->CreateIntCast(Res, Type::getInt32Ty(TheContext), true);
        genStoreRegister(Inst.RD, ResCasted, Func);
        break;
      }

    case dbt::OIDecoder::Movz: {  
        Value* Res = Builder->CreateICmpEQ(genLoadRegister(Inst.RT, Func), genImm(0));
        BasicBlock* TBB = BasicBlock::Create(TheContext, "", Func);
        BasicBlock* FBB = BasicBlock::Create(TheContext, "", Func);
        BranchInst* Br = Builder->CreateCondBr(Res, TBB, FBB);
        Builder->SetInsertPoint(TBB);
        genStoreRegister(Inst.RD, genLoadRegister(Inst.RS, Func), Func);
        Builder->CreateBr(FBB);
        Builder->SetInsertPoint(FBB);
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

    case dbt::OIDecoder::Jnez: {
        BasicBlock* BB = BasicBlock::Create(TheContext, "", Func);
        Value* Res = Builder->CreateICmpNE(genLoadRegister(Inst.RS, Func), genImm(0));
        BranchInst* Br = Builder->CreateCondBr(Res, BB, BB);
        Builder->SetInsertPoint(BB);
        IRBranchMap[GuestAddr] = Br;
        break;
      }

    case dbt::OIDecoder::Jeq: {
        BasicBlock* BB = BasicBlock::Create(TheContext, "", Func);
        Value* Res = Builder->CreateICmpEQ(genLoadRegister(Inst.RS, Func), genLoadRegister(Inst.RT, Func));
        BranchInst* Br = Builder->CreateCondBr(Res, BB, BB);
        Builder->SetInsertPoint(BB);
        IRBranchMap[GuestAddr] = Br;
        break;
      }

    case dbt::OIDecoder::Jlez: {
        BasicBlock* BB = BasicBlock::Create(TheContext, "", Func);
        Value* Res1 = Builder->CreateICmpEQ(genLoadRegister(Inst.RT, Func), genImm(0));
        Value* Res2 = Builder->CreateAnd(genLoadRegister(Inst.RT, Func), 0x80000000);
        Value* Res  = genLogicalOr(Res1, Res2, Func); 
        BranchInst* Br = Builder->CreateCondBr(Res, BB, BB);
        Builder->SetInsertPoint(BB);
        IRBranchMap[GuestAddr] = Br;
        break;
      }

    case dbt::OIDecoder::Jltz: {
        BasicBlock* BB = BasicBlock::Create(TheContext, "", Func);
        Value* Res = Builder->CreateAnd(genLoadRegister(Inst.RT, Func), 0x80000000);
        BranchInst* Br = Builder->CreateCondBr(Res, BB, BB);
        Builder->SetInsertPoint(BB);
        IRBranchMap[GuestAddr] = Br;
        break;
      }

    case dbt::OIDecoder::Jgtz: {
        BasicBlock* BB = BasicBlock::Create(TheContext, "", Func);
        Value* Res1 = Builder->CreateAnd(genLoadRegister(Inst.RT, Func), 0x80000000);
        Value* Res2 = Builder->CreateICmpNE(genLoadRegister(Inst.RT, Func), genImm(0));
        Value* Res  = genLogicalAnd(Res1, Res2, Func); 
        BranchInst* Br = Builder->CreateCondBr(Builder->CreateNot(Res), BB, BB);
        Builder->SetInsertPoint(BB);
        IRBranchMap[GuestAddr] = Br;
        break;
      }

    case dbt::OIDecoder::Jne: {
        BasicBlock* BB = BasicBlock::Create(TheContext, "", Func);
        Value* Res = Builder->CreateICmpNE(genLoadRegister(Inst.RS, Func), genLoadRegister(Inst.RT, Func));
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
    default: {
        std::cout << "Mother of God! We don't have support to emit inst at: " << std::hex << GuestAddr << " (" <<
          dbt::OIPrinter::getString(Inst) << ")\n";
        exit(1);
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
  std::vector<BasicBlock*> TrashBlocks;
  for (auto& BB : *F) 
    if (BB.size() == 0 || (pred_empty(&BB) && BB.getName() != "entry")) 
      TrashBlocks.push_back(&BB);

  for (auto& BB : TrashBlocks)
    BB->eraseFromParent();
}

void dbt::IREmitter::processBranchesTargets(const OIInstList& OIRegion) {
  for (auto Pair : OIRegion) {
    OIDecoder::OIInst Inst = OIDecoder::decode(Pair[1]);
    uint32_t GuestAddr = Pair[0];

    switch (Inst.Type) {
      case dbt::OIDecoder::Jne: 
      case dbt::OIDecoder::Jeqz: 
      case dbt::OIDecoder::Jlez:
      case dbt::OIDecoder::Jltz:
      case dbt::OIDecoder::Jnez:
      case dbt::OIDecoder::Jgtz:
      case dbt::OIDecoder::Jeq: 
        {
          updateBranchTarget(GuestAddr, {(GuestAddr + (Inst.Imm << 2)) + 4, GuestAddr + 4});
          break;
        }
      case dbt::OIDecoder::Jump: 
      case dbt::OIDecoder::Call: 
        {
          updateBranchTarget(GuestAddr, {(GuestAddr & 0xF0000000) | (Inst.Addrs << 2), 0});
          break;
        }
    }
  }
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

  cleanCFG();

  return TheModule;
}
