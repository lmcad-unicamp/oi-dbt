#include <IREmitter.hpp>
#include <syscallIREmitter.hpp>
#include <OIPrinter.hpp>

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/Type.h"
#include "llvm/ExecutionEngine/MCJIT.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm-c/Core.h"
#include "llvm-c/Disassembler.h"

#include <cstdlib>
#include <stddef.h>
#include <fstream>
#include <sstream>
#include <iomanip>

using namespace llvm;

void dbt::IREmitter::generateInstIR(const uint32_t GuestAddr, const dbt::OIDecoder::OIInst Inst) {
  Function* Func = Builder->GetInsertBlock()->getParent();
  LLVMContext& C = Func->getContext();
  int instructions = 10;
  auto lastFuncInst = inst_end(Func);
  //SyscallIREmitter syscallIR(*this);

  // If the address is not continuos and the last inst was not a jump, split the BB
  std::array<uint32_t, 2> Targets = getPossibleTargets(LastEmittedAddrs, LastEmittedInst);
  if (LastEmittedAddrs != 0 && Targets[0] != GuestAddr && Targets[1] != GuestAddr) {
    if (Trampoline != nullptr) {
      Builder->CreateStore(genImm(LastEmittedAddrs+4), ReturnAddrs);
      Builder->CreateBr(Trampoline);
    } else {
      insertDirectExit(genImm(LastEmittedAddrs+4));
    }

    BasicBlock* BB = BasicBlock::Create(TheContext, "", Func);
    Builder->SetInsertPoint(BB);
  }

  switch (Inst.Type) {
    case dbt::OIDecoder::Nop: {
        Function *fun = Intrinsic::getDeclaration(Func->getParent(), Intrinsic::donothing);
        Value *Res = Builder->CreateCall(fun);
        setIfNotTheFirstInstGen(Res);
        break;
      }

    case dbt::OIDecoder::Ldi: {
        ldireg = Inst.RT;
        genStoreRegister(64, genImm(Inst.RT), Func); 
        Value* R1 = Builder->CreateAnd(genLoadRegister(Inst.RT, Func), genImm(0xFFFFC000));
        Value* Res = Builder->CreateOr(R1, genImm(Inst.Imm & 0x3FFF));
        genStoreRegister(Inst.RT, Res, Func);
        break;
      }

    case dbt::OIDecoder::Ldihi: {
        Value* R1 = Builder->CreateAnd(genLoadRegister(ldireg, Func), genImm(0x3FFF));
        Value* Res = Builder->CreateOr(R1, genImm(Inst.Addrs << 14));
        genStoreRegister(ldireg, Res, Func);
        break;
      }

    case dbt::OIDecoder::Ori: {
        Value* Res = Builder->CreateOr(genLoadRegister(Inst.RS, Func), genImm(Inst.Imm & 0x3FFF));
        genStoreRegister(Inst.RT, Res, Func);
        break;
      }

    case dbt::OIDecoder::Ror: {
				Value* Input = genLoadRegister(Inst.RT, Func);
				Value* S1 = Builder->CreateLShr(Input, genImm(Inst.RS));
				Value* S2 = Builder->CreateShl(Input, genImm(32-Inst.RS));
        Value* Res = Builder->CreateOr(S1, S2);
        genStoreRegister(Inst.RD, Res, Func);
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

    case dbt::OIDecoder::Modu: {
        Value* Res = Builder->CreateURem(genLoadRegister(Inst.RS, Func), genLoadRegister(Inst.RT, Func));
        genStoreRegister(Inst.RV, Res, Func);
        break;
      }

    case dbt::OIDecoder::Shl: {
        Value* Res = Builder->CreateShl(genLoadRegister(Inst.RT, Func), genImm(Inst.RS));
        genStoreRegister(Inst.RD, Res, Func);
        break;
      }

    case dbt::OIDecoder::Shlr: {
        Value* Shifted = Builder->CreateAnd(genLoadRegister(Inst.RS, Func), genImm(0x1F));
        Value* Res = Builder->CreateShl(genLoadRegister(Inst.RT, Func), Shifted);
        genStoreRegister(Inst.RD, Res, Func);
        break;
      }

    case dbt::OIDecoder::Shrr: {
        Value* RS = genLoadRegister(Inst.RS, Func);
        Value* RShift = Builder->CreateAnd(RS, 0x1F);
        Value* Res = Builder->CreateLShr(genLoadRegister(Inst.RT, Func), RShift);
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

    case dbt::OIDecoder::Asrr: {
        Value* RS = genLoadRegister(Inst.RS, Func);
        Value* And = Builder->CreateAnd(RS, genImm(0x1F));
        Value* Res = Builder->CreateAShr(genLoadRegister(Inst.RT, Func), And);
        genStoreRegister(Inst.RD, Res, Func);
        break;
      }

    case dbt::OIDecoder::Div: {
        Value* Res = Builder->CreateExactSDiv(genLoadRegister(Inst.RS, Func), genLoadRegister(Inst.RT, Func));
        genStoreRegister(Inst.RD, Res, Func);
        break;
      }

    case dbt::OIDecoder::Divu: {
        Value *RS = Builder->CreateIntCast(genLoadRegister(Inst.RS, Func), Type::getInt64Ty(TheContext), true);
        Value *RT = Builder->CreateIntCast(genLoadRegister(Inst.RT, Func), Type::getInt64Ty(TheContext), true);
        Value* Res = Builder->CreateExactUDiv(genLoadRegister(Inst.RS, Func), genLoadRegister(Inst.RT, Func));

        //RD 0 raises exception
        genStoreRegister(Inst.RD, Res, Func);

         if (Inst.RV != 0) {
             Value* ResMod = Builder->CreateSRem(genLoadRegister(Inst.RS, Func), genLoadRegister(Inst.RT, Func));
             genStoreRegister(Inst.RV, ResMod, Func);
         }

        break;
      }

    case dbt::OIDecoder::Mul: {
        Value *RS = Builder->CreateIntCast(genLoadRegister(Inst.RS, Func), Type::getInt64Ty(TheContext), true);
        Value *RT = Builder->CreateIntCast(genLoadRegister(Inst.RT, Func), Type::getInt64Ty(TheContext), true);
        Value* Res = Builder->CreateMul(RS, RT);
        if (Inst.RD != 0) {
          Value* ShiftedRes = Builder->CreateAnd(Res, genImm(0xFFFFFFFF));
          genStoreRegister(Inst.RD, Builder->CreateIntCast(ShiftedRes, Type::getInt32Ty(TheContext), true), Func);
        }
        if (Inst.RV != 0) {
          Value* ShiftedRes = Builder->CreateAnd(Builder->CreateAShr(Res, 32), genImm(0xFFFFFFFF));
          genStoreRegister(Inst.RV, Builder->CreateIntCast(ShiftedRes, Type::getInt32Ty(TheContext), true), Func);
        }
        break;
      }

    case dbt::OIDecoder::Mulu: {
        Value* URS = Builder->CreateIntCast(genLoadRegister(Inst.RS, Func), Type::getInt64Ty(TheContext), false);
        Value* URT = Builder->CreateIntCast(genLoadRegister(Inst.RT, Func), Type::getInt64Ty(TheContext), false);
        Value* Res = Builder->CreateNUWMul(URS, URT);
        if (Inst.RD != 0) {
          Value* UShiftedRes = Builder->CreateAnd(Res, genImm(0xFFFFFFFF));
          Value* ShiftedRes = Builder->CreateIntCast(UShiftedRes, Type::getInt32Ty(TheContext), false);
          genStoreRegister(Inst.RD, ShiftedRes, Func);
        }
        if (Inst.RV != 0) {
          Value* UShiftedRes = Builder->CreateAnd(Builder->CreateLShr(Res, 32), genImm(0xFFFFFFFF));
          Value* ShiftedRes = Builder->CreateIntCast(UShiftedRes, Type::getInt32Ty(TheContext), false);
          genStoreRegister(Inst.RV, ShiftedRes, Func);
        }
        setIfNotTheFirstInstGen(URS);
        break;
      }

    case dbt::OIDecoder::Xor: {
        Value* Res = Builder->CreateXor(genLoadRegister(Inst.RS, Func), genLoadRegister(Inst.RT, Func));
        genStoreRegister(Inst.RD, Res, Func);
        setIfNotTheFirstInstGen(Res);
        break;
      }

    case dbt::OIDecoder::Or: {
        Value* Res = Builder->CreateOr(genLoadRegister(Inst.RS, Func), genLoadRegister(Inst.RT, Func));
        genStoreRegister(Inst.RD, Res, Func);
        setIfNotTheFirstInstGen(Res);
        break;
      }

    case dbt::OIDecoder::Nor: {
        Value* Res = Builder->CreateOr(genLoadRegister(Inst.RS, Func), genLoadRegister(Inst.RT, Func));
        genStoreRegister(Inst.RD, Builder->CreateNot(Res), Func);
        setIfNotTheFirstInstGen(Res);
        break;
      }

    case dbt::OIDecoder::And: {
        Value* Res = Builder->CreateAnd(genLoadRegister(Inst.RS, Func), genLoadRegister(Inst.RT, Func));
        genStoreRegister(Inst.RD, Res, Func);
        setIfNotTheFirstInstGen(Res);
        break;
      }

    case dbt::OIDecoder::Andi: {
        Value* Res = Builder->CreateAnd(genLoadRegister(Inst.RS, Func), genImm(Inst.Imm & 0x3FFF));
        genStoreRegister(Inst.RT, Res, Func);
        setIfNotTheFirstInstGen(Res);
        break;
    }

    case dbt::OIDecoder::Sub: {
        Value* Res = Builder->CreateSub(genLoadRegister(Inst.RS, Func), genLoadRegister(Inst.RT, Func));
        genStoreRegister(Inst.RD, Res, Func);
        setIfNotTheFirstInstGen(Res);
        break;
      }

    case dbt::OIDecoder::Add: {
        Value* Res = Builder->CreateAdd(genLoadRegister(Inst.RS, Func), genLoadRegister(Inst.RT, Func));
        genStoreRegister(Inst.RD, Res, Func);
        setIfNotTheFirstInstGen(Res);
        break;
      }

    case dbt::OIDecoder::Addi: {
        Value* Res = Builder->CreateAdd(genLoadRegister(Inst.RS, Func), genImm(Inst.Imm));
        genStoreRegister(Inst.RT, Res, Func);
        setIfNotTheFirstInstGen(Res);
        break;
      }

    case dbt::OIDecoder::Ldw: {
        Value* RawAddrs = Builder->CreateAdd(genLoadRegister(Inst.RS, Func), genImm(Inst.Imm));
        Value* Res = Builder->CreateLoad(genDataWordVecPtr(RawAddrs, Func));
        genStoreRegister(Inst.RT, Res, Func);
        setIfNotTheFirstInstGen(RawAddrs);
        break;
      }

    case dbt::OIDecoder::Ldh: {
        Value* RawAddrs = Builder->CreateAdd(genLoadRegister(Inst.RS, Func), genImm(Inst.Imm));
        Value* Res = Builder->CreateLoad(genDataHalfVecPtr(RawAddrs, Func));
        genStoreRegister(Inst.RT, Builder->CreateIntCast(Res, Type::getInt32Ty(TheContext), true), Func);
        setIfNotTheFirstInstGen(RawAddrs);
        break;
    }

    case dbt::OIDecoder::Ldhu: {
        Value* RawAddrs = Builder->CreateAdd(genLoadRegister(Inst.RS, Func), genImm(Inst.Imm));
        Value* Res = Builder->CreateLoad(genDataHalfVecPtr(RawAddrs, Func));
        genStoreRegister(Inst.RT, Builder->CreateIntCast(Res, Type::getInt32Ty(TheContext), false), Func);
        break;
    }

    case dbt::OIDecoder::Ldb: {
        Value* RawAddrs = Builder->CreateAdd(genLoadRegister(Inst.RS, Func), genImm(Inst.Imm));
        Value* Res = Builder->CreateLoad(genDataByteVecPtr(RawAddrs, Func));
        genStoreRegister(Inst.RT, Builder->CreateIntCast(Res, Type::getInt32Ty(TheContext), true), Func);
        break;
      }

    case dbt::OIDecoder::Ldbu: {
        Value* RawAddrs = Builder->CreateAdd(genLoadRegister(Inst.RS, Func), genImm(Inst.Imm));
        Value* Res = Builder->CreateLoad(genDataByteVecPtr(RawAddrs, Func));
        genStoreRegister(Inst.RT, Builder->CreateIntCast(Res, Type::getInt32Ty(TheContext), false), Func);
        break;
      }

    case dbt::OIDecoder::Stw: {
        Value* RawAddrs = Builder->CreateAdd(genLoadRegister(Inst.RS, Func), genImm(Inst.Imm));
        Value* Res = Builder->CreateStore(genLoadRegister(Inst.RT, Func), genDataWordVecPtr(RawAddrs, Func));
        break;
      }

    case dbt::OIDecoder::Sth: {
        Value* And1 = Builder->CreateAnd(genLoadRegister(Inst.RT, Func), 0xFFFF);
        Value* Half = Builder->CreateIntCast(And1, Type::getInt16Ty(TheContext), false);
        Value* Addrs = Builder->CreateAdd(genLoadRegister(Inst.RS, Func), genImm(Inst.Imm));
        Value* Res = Builder->CreateStore(Half, genDataHalfVecPtr(Addrs, Func));
        break;
      }

    case dbt::OIDecoder::Seb: {
        Value* Res = Builder->CreateIntCast(genLoadRegister(Inst.RT, Func), Type::getInt8Ty(TheContext), true);
        genStoreRegister(Inst.RS, Builder->CreateIntCast(Res, Type::getInt32Ty(TheContext), true), Func);
        break;
    }

    case dbt::OIDecoder::Seh: {
        Value* Res = Builder->CreateIntCast(
            Builder->CreateIntCast(genLoadRegister(Inst.RT, Func), Type::getInt16Ty(TheContext), true),
            Type::getInt32Ty(TheContext), true);
        genStoreRegister(Inst.RS, Res, Func);
        break;
    }

    case dbt::OIDecoder::Stb: {
        Value* RawAddrs = Builder->CreateAdd(genLoadRegister(Inst.RS, Func), genImm(Inst.Imm));
        Value* RT = Builder->CreateAnd(genLoadRegister(Inst.RT, Func), genImm(0xFF));
        Value* UChar = Builder->CreateIntCast(RT, Type::getInt8Ty(TheContext), false);
        Value* Res = Builder->CreateStore(UChar, genDataByteVecPtr(RawAddrs, Func));
        break;
      }

    case dbt::OIDecoder::Slti: {
        Value* Res = Builder->CreateICmpSLT(genLoadRegister(Inst.RS, Func), genImm(Inst.Imm));
        Value* ResCasted = Builder->CreateZExt(Res, Type::getInt32Ty(TheContext));
        genStoreRegister(Inst.RT, ResCasted, Func);
        break;
      }

    case dbt::OIDecoder::Slt: {
        Value* Res = Builder->CreateICmpSLT(genLoadRegister(Inst.RS, Func), genLoadRegister(Inst.RT, Func));
        Value* ResCasted = Builder->CreateZExt(Res, Type::getInt32Ty(TheContext));
        genStoreRegister(Inst.RD, ResCasted, Func);
        break;
      }

    case dbt::OIDecoder::Sltiu: {
        Value* Res = Builder->CreateICmpULT(genLoadRegister(Inst.RS, Func), genImm(Inst.Imm & 0x3FFF));
        Value* ResCasted = Builder->CreateZExt(Res, Type::getInt32Ty(TheContext));
        genStoreRegister(Inst.RT, ResCasted, Func);
        break;
      }

    case dbt::OIDecoder::Sltu: {
        Value* Res = Builder->CreateICmpULT(genLoadRegister(Inst.RS, Func), genLoadRegister(Inst.RT, Func));
        Value* ResCasted = Builder->CreateZExt(Res, Type::getInt32Ty(TheContext));
        genStoreRegister(Inst.RD, ResCasted, Func);
        break;
      }

    case dbt::OIDecoder::Movz: {
        Value* Res = Builder->CreateICmpEQ(genLoadRegister(Inst.RT, Func), genImm(0));
        BasicBlock* TBB = BasicBlock::Create(TheContext, "", Func);
        BasicBlock* FBB = BasicBlock::Create(TheContext, "", Func);
        Builder->CreateCondBr(Res, TBB, FBB);
        Builder->SetInsertPoint(TBB);
        genStoreRegister(Inst.RD, genLoadRegister(Inst.RS, Func), Func);
        Builder->CreateBr(FBB);
        Builder->SetInsertPoint(FBB);
        break;
      }

    case dbt::OIDecoder::Movzd: {
        Value* Res = Builder->CreateICmpEQ(genLoadRegister(Inst.RT, Func), genImm(0));
        BasicBlock* TBB = BasicBlock::Create(TheContext, "", Func);
        BasicBlock* FBB = BasicBlock::Create(TheContext, "", Func);
        Builder->CreateCondBr(Res, TBB, FBB);
        Builder->SetInsertPoint(TBB);
        genStoreRegister(Inst.RD, genLoadRegister(Inst.RS, Func, RegType::Double), Func, RegType::Double);
        Builder->CreateBr(FBB);
        Builder->SetInsertPoint(FBB);
        break;
      }

    case dbt::OIDecoder::Movn: {
        Value* Res = Builder->CreateICmpNE(genLoadRegister(Inst.RT, Func), genImm(0));
        BasicBlock* TBB = BasicBlock::Create(TheContext, "", Func);
        BasicBlock* FBB = BasicBlock::Create(TheContext, "", Func);
        Builder->CreateCondBr(Res, TBB, FBB);
        Builder->SetInsertPoint(TBB);
        genStoreRegister(Inst.RD, genLoadRegister(Inst.RS, Func), Func);
        Builder->CreateBr(FBB);
        Builder->SetInsertPoint(FBB);
        break;
      }

    case dbt::OIDecoder::Movnd: {
        Value* Res = Builder->CreateICmpNE(genLoadRegister(Inst.RT, Func), genImm(0));
        BasicBlock* TBB = BasicBlock::Create(TheContext, "", Func);
        BasicBlock* FBB = BasicBlock::Create(TheContext, "", Func);
        Builder->CreateCondBr(Res, TBB, FBB);
        Builder->SetInsertPoint(TBB);
        genStoreRegister(Inst.RD, genLoadRegister(Inst.RS, Func, RegType::Double), Func, RegType::Double);
        Builder->CreateBr(FBB);
        Builder->SetInsertPoint(FBB);
        break;
      }

    case dbt::OIDecoder::Ijmphi: {
        Value* Res = Builder->CreateOr(genImm(0), genImm(Inst.Addrs << 12));
        genStoreRegister(IJMP_REG, Res, Func, RegType::Int);
        break;
      }

//USub?
    case dbt::OIDecoder::Ext: {
        Value* R1 = Builder->CreateSub(genImm(32), genImm(Inst.RS + Inst.RT + 1));
        Value* R2 = Builder->CreateShl(genLoadRegister(Inst.RD, Func), R1);
        Value* R3 = Builder->CreateSub(genImm(32), genImm(Inst.RT + 1));
        Value* R4 = Builder->CreateLShr(R2, R3);
        genStoreRegister(Inst.RV, R4, Func, RegType::Int);
        setIfNotTheFirstInstGen(R1);
        break;
      }

    /************************************** FLOAT *****************************************/

    //A
    case dbt::OIDecoder::Movd: {
        Value* Value = genLoadRegister(Inst.RT, Func, RegType::Double);
        genStoreRegister(Inst.RS, Value, Func, RegType::Double);
        break;
      }

    //A
    case dbt::OIDecoder::Movs: {
        Value* Value = genLoadRegister(Inst.RT, Func, RegType::Float);
        genStoreRegister(Inst.RS, Value, Func, RegType::Float);
        break;
      }

    //A
    case dbt::OIDecoder::Movt: {
        Value* Res = Builder->CreateICmpNE(genLoadRegister(CC_REG, Func), genImm(0));
        BasicBlock* TBB = BasicBlock::Create(TheContext, "", Func);
        BasicBlock* FBB = BasicBlock::Create(TheContext, "", Func);
        Builder->CreateCondBr(Res, TBB, FBB);
        Builder->SetInsertPoint(TBB);
        genStoreRegister(Inst.RS, genLoadRegister(Inst.RT, Func), Func);
        Builder->CreateBr(FBB);
        Builder->SetInsertPoint(FBB);
        break;
      }

      //A
    case dbt::OIDecoder::Movf: {
        Value* Res = Builder->CreateICmpEQ(genLoadRegister(CC_REG, Func), genImm(0));
        BasicBlock* TBB = BasicBlock::Create(TheContext, "", Func);
        BasicBlock* FBB = BasicBlock::Create(TheContext, "", Func);
        Builder->CreateCondBr(Res, TBB, FBB);
        Builder->SetInsertPoint(TBB);
        genStoreRegister(Inst.RS, genLoadRegister(Inst.RT, Func), Func);
        Builder->CreateBr(FBB);
        Builder->SetInsertPoint(FBB);
        break;
      }

      //A? Or double?
    case dbt::OIDecoder::Movfd: {
        Value* Res = Builder->CreateICmpEQ(genLoadRegister(CC_REG, Func), genImm(0));
        BasicBlock* TBB = BasicBlock::Create(TheContext, "", Func);
        BasicBlock* FBB = BasicBlock::Create(TheContext, "", Func);
        Builder->CreateCondBr(Res, TBB, FBB);
        Builder->SetInsertPoint(TBB);
        genStoreRegister(Inst.RS, genLoadRegister(Inst.RT, Func, RegType::Double), Func, RegType::Double);
        Builder->CreateBr(FBB);
        Builder->SetInsertPoint(FBB);
        break;
      }

    case dbt::OIDecoder::Movfs: {
        Value* Res = Builder->CreateICmpEQ(genLoadRegister(CC_REG, Func), genImm(0));
        BasicBlock* TBB = BasicBlock::Create(TheContext, "", Func);
        BasicBlock* FBB = BasicBlock::Create(TheContext, "", Func);
        Builder->CreateCondBr(Res, TBB, FBB);
        Builder->SetInsertPoint(TBB);
        genStoreRegister(Inst.RS, genLoadRegister(Inst.RT, Func, RegType::Float), Func, RegType::Float);
        Builder->CreateBr(FBB);
        Builder->SetInsertPoint(FBB);
        break;
      }

      //A? Or double?
    case dbt::OIDecoder::Movts: {
        Value* Res = Builder->CreateICmpNE(genLoadRegister(CC_REG, Func), genImm(0));
        BasicBlock* TBB = BasicBlock::Create(TheContext, "", Func);
        BasicBlock* FBB = BasicBlock::Create(TheContext, "", Func);
        Builder->CreateCondBr(Res, TBB, FBB);
        Builder->SetInsertPoint(TBB);
        genStoreRegister(Inst.RS, genLoadRegister(Inst.RT, Func, RegType::Float), Func, RegType::Float);
        Builder->CreateBr(FBB);
        Builder->SetInsertPoint(FBB);
        break;
      }

    case dbt::OIDecoder::Movtd: {
        Value* Res = Builder->CreateICmpNE(genLoadRegister(CC_REG, Func), genImm(0));
        BasicBlock* TBB = BasicBlock::Create(TheContext, "", Func);
        BasicBlock* FBB = BasicBlock::Create(TheContext, "", Func);
        Builder->CreateCondBr(Res, TBB, FBB);
        Builder->SetInsertPoint(TBB);
        genStoreRegister(Inst.RS, genLoadRegister(Inst.RT, Func, RegType::Double), Func, RegType::Double);
        Builder->CreateBr(FBB);
        Builder->SetInsertPoint(FBB);
        break;
      }

      //A
    case dbt::OIDecoder::Cvtds: {
        Value* FloatValue  = genLoadRegister(Inst.RT, Func, RegType::Float);
        Value* DoubleValue = Builder->CreateFPExt(FloatValue, Type::getDoubleTy(TheContext));
        genStoreRegister(Inst.RS, DoubleValue, Func, RegType::Double);
        break;
      }

      //A?
    case dbt::OIDecoder::Cvtdw: {
        Value* FloatValue  = genLoadRegister(Inst.RT, Func, RegType::Float);
        Value* IntValue = Builder->CreateBitCast(FloatValue, Type::getInt32Ty(TheContext));
        Value* DoubleValue = Builder->CreateSIToFP(IntValue, Type::getDoubleTy(TheContext));
        genStoreRegister(Inst.RS, DoubleValue, Func, RegType::Double);
        break;
      }

      //A
    case dbt::OIDecoder::Cvtsw: {
        Value* RT  = genLoadRegister(Inst.RT, Func, RegType::Float);
        Value* Int = Builder->CreateBitCast(RT, Type::getInt32Ty(TheContext));
        Value* Float = Builder->CreateSIToFP(Int, Type::getFloatTy(TheContext));
        genStoreRegister(Inst.RS, Float, Func, RegType::Float);
        break;
      }

      //A? FP cast?
    case dbt::OIDecoder::Cvtsd: {
        Value* RT  = genLoadRegister(Inst.RT, Func, RegType::Double);
        Value* Float = Builder->CreateFPCast(RT, Type::getFloatTy(TheContext));
        genStoreRegister(Inst.RS, Float, Func, RegType::Float);
        break;
      }

     case dbt::OIDecoder::Cueqd: {
        Value* A = genLoadRegister(Inst.RS, Func, RegType::Double);
        Value* B = genLoadRegister(Inst.RT, Func, RegType::Double);
        Value* Res = Builder->CreateFCmpUEQ(A, B);
        genStoreRegister(CC_REG, Builder->CreateZExt(Res, Type::getInt32Ty(TheContext)), Func, RegType::Int);
        break;
      }

     case dbt::OIDecoder::Ceqd: {
        Value* A = genLoadRegister(Inst.RS, Func, RegType::Double);
        Value* B = genLoadRegister(Inst.RT, Func, RegType::Double);
        Value* Res = Builder->CreateFCmpOEQ(A, B);
        genStoreRegister(CC_REG, Builder->CreateZExt(Res, Type::getInt32Ty(TheContext)), Func, RegType::Int);
        break;
      }

     case dbt::OIDecoder::Ceqs: {
        Value* A = genLoadRegister(Inst.RS, Func, RegType::Float);
        Value* B = genLoadRegister(Inst.RT, Func, RegType::Float);
        Value* Res = Builder->CreateFCmpOEQ(A, B);
        genStoreRegister(CC_REG, Builder->CreateZExt(Res, Type::getInt32Ty(TheContext)), Func, RegType::Int);
        break;
      }

     case dbt::OIDecoder::Cults: {
        Value* A = genLoadRegister(Inst.RS, Func, RegType::Float);
        Value* B = genLoadRegister(Inst.RT, Func, RegType::Float);
        Value* Res = Builder->CreateFCmpULT(A, B);
        genStoreRegister(CC_REG, Builder->CreateZExt(Res, Type::getInt32Ty(TheContext)), Func, RegType::Int);
        break;
      }

     case dbt::OIDecoder::Cultd: {
        Value* A = genLoadRegister(Inst.RS, Func, RegType::Double);
        Value* B = genLoadRegister(Inst.RT, Func, RegType::Double);
        Value* Res = Builder->CreateFCmpULT(A, B);
        genStoreRegister(CC_REG, Builder->CreateZExt(Res, Type::getInt32Ty(TheContext)), Func, RegType::Int);
        break;
      }

     case dbt::OIDecoder::Culed: {
        Value* A = genLoadRegister(Inst.RS, Func, RegType::Double);
        Value* B = genLoadRegister(Inst.RT, Func, RegType::Double);
        Value* Res = Builder->CreateFCmpULE(A, B);
        genStoreRegister(CC_REG, Builder->CreateZExt(Res, Type::getInt32Ty(TheContext)), Func, RegType::Int);
        break;
      }

     case dbt::OIDecoder::Cules: {
        Value* A = genLoadRegister(Inst.RS, Func, RegType::Float);
        Value* B = genLoadRegister(Inst.RT, Func, RegType::Float);
        Value* Res = Builder->CreateFCmpULE(A, B);
        genStoreRegister(CC_REG, Builder->CreateZExt(Res, Type::getInt32Ty(TheContext)), Func, RegType::Int);
        break;
      }

    case dbt::OIDecoder::Cund: {
        Value* A = genLoadRegister(Inst.RS, Func, RegType::Double);
        Value* B = genLoadRegister(Inst.RT, Func, RegType::Double);
        Value* Res = Builder->CreateFCmpUNO(A, B); //FIXME: Isn't a ||?
        genStoreRegister(CC_REG, Builder->CreateZExt(Res, Type::getInt32Ty(TheContext)), Func, RegType::Int);
        break;
      }

    case dbt::OIDecoder::Cuns: {
        Value* A = genLoadRegister(Inst.RS, Func, RegType::Float);
        Value* B = genLoadRegister(Inst.RT, Func, RegType::Float);
        Value* Res = Builder->CreateFCmpUNO(A, B); //FIXME: Isn't a ||?
        genStoreRegister(CC_REG, Builder->CreateZExt(Res, Type::getInt32Ty(TheContext)), Func, RegType::Int);
        break;
      }

    case dbt::OIDecoder::Coled: {
        Value* A = genLoadRegister(Inst.RS, Func, RegType::Double);
        Value* B = genLoadRegister(Inst.RT, Func, RegType::Double);
        Value* Res = Builder->CreateFCmpOLE(A, B);
        genStoreRegister(CC_REG, Builder->CreateZExt(Res, Type::getInt32Ty(TheContext)), Func, RegType::Int);
        break;
      }

    case dbt::OIDecoder::Coles: {
        Value* A = genLoadRegister(Inst.RS, Func, RegType::Float);
        Value* B = genLoadRegister(Inst.RT, Func, RegType::Float);
        Value* Res = Builder->CreateFCmpOLE(A, B);
        genStoreRegister(CC_REG, Builder->CreateZExt(Res, Type::getInt32Ty(TheContext)), Func, RegType::Int);
        break;
      }

    case dbt::OIDecoder::Coltd: {
        Value* A = genLoadRegister(Inst.RS, Func, RegType::Double);
        Value* B = genLoadRegister(Inst.RT, Func, RegType::Double);
        Value* Res = Builder->CreateFCmpOLT(A, B);
        genStoreRegister(CC_REG, Builder->CreateZExt(Res, Type::getInt32Ty(TheContext)), Func, RegType::Int);
        break;
      }

    case dbt::OIDecoder::Colts: {
        Value* A = genLoadRegister(Inst.RS, Func, RegType::Float);
        Value* B = genLoadRegister(Inst.RT, Func, RegType::Float);
        Value* Res = Builder->CreateFCmpOLT(A, B);
        genStoreRegister(CC_REG, Builder->CreateZExt(Res, Type::getInt32Ty(TheContext)), Func, RegType::Int);
        break;
      }

    case dbt::OIDecoder::Negs: {
        Value* RT  = genLoadRegister(Inst.RT, Func, RegType::Float);
        Value* Res = Builder->CreateFNeg(RT);
        genStoreRegister(Inst.RS, Res, Func, RegType::Float);
        break;
      }

    case dbt::OIDecoder::Negd: {
        Value* RT  = genLoadRegister(Inst.RT, Func, RegType::Double);
        Value* Res = Builder->CreateFNeg(RT);
        genStoreRegister(Inst.RS, Res, Func, RegType::Double);
        break;
      }

    case dbt::OIDecoder::Muld: {
        Value* RS  = genLoadRegister(Inst.RS, Func, RegType::Double);
        Value* RT  = genLoadRegister(Inst.RT, Func, RegType::Double);
        Value* Res = Builder->CreateFMul(RS, RT);
        genStoreRegister(Inst.RD, Res, Func, RegType::Double);
        break;
      }

    case dbt::OIDecoder::Muls: {
        Value* RS  = genLoadRegister(Inst.RS, Func, RegType::Float);
        Value* RT  = genLoadRegister(Inst.RT, Func, RegType::Float);
        Value* Res = Builder->CreateFMul(RS, RT);
        genStoreRegister(Inst.RD, Res, Func, RegType::Float);
        break;
      }

    case dbt::OIDecoder::Divd: {
        Value* RS  = genLoadRegister(Inst.RS, Func, RegType::Double);
        Value* RT  = genLoadRegister(Inst.RT, Func, RegType::Double);
        Value* Res = Builder->CreateFDiv(RS, RT);
        genStoreRegister(Inst.RD, Res, Func, RegType::Double);
        break;
      }

    case dbt::OIDecoder::Divs: {
        Value* RS  = genLoadRegister(Inst.RS, Func, RegType::Float);
        Value* RT  = genLoadRegister(Inst.RT, Func, RegType::Float);
        Value* Res = Builder->CreateFDiv(RS, RT);
        genStoreRegister(Inst.RD, Res, Func, RegType::Float);
        break;
      }


    case dbt::OIDecoder::Adds: {
        Value* RS  = genLoadRegister(Inst.RS, Func, RegType::Float);
        Value* RT  = genLoadRegister(Inst.RT, Func, RegType::Float);
        Value* Res = Builder->CreateFAdd(RS, RT);
        genStoreRegister(Inst.RD, Res, Func, RegType::Float);
        break;
      }

    case dbt::OIDecoder::Absd: {
        std::vector<Type *> arg_type;
        arg_type.push_back(Type::getDoubleTy(TheContext));
        Function *fun = Intrinsic::getDeclaration(Func->getParent(), Intrinsic::fabs, arg_type);
        Value* RT  = genLoadRegister(Inst.RT, Func, RegType::Double);
        Value *Res = Builder->CreateCall(fun, {RT});
        genStoreRegister(Inst.RS, Res, Func, RegType::Double);
        break;
      }

    case dbt::OIDecoder::Abss: {
        std::vector<Type *> arg_type;
        arg_type.push_back(Type::getFloatTy(TheContext));
        Function *fun = Intrinsic::getDeclaration(Func->getParent(), Intrinsic::fabs, arg_type);
        Value* RT  = genLoadRegister(Inst.RT, Func, RegType::Float);
        Value *Res = Builder->CreateCall(fun, {RT});
        genStoreRegister(Inst.RS, Res, Func, RegType::Float);
        break;
      }

    case dbt::OIDecoder::Addd: {
        Value* RS  = genLoadRegister(Inst.RS, Func, RegType::Double);
        Value* RT  = genLoadRegister(Inst.RT, Func, RegType::Double);
        Value* Res = Builder->CreateFAdd(RS, RT);
        genStoreRegister(Inst.RD, Res, Func, RegType::Double);
        break;
      }

    case dbt::OIDecoder::Subs: {
        Value* RS  = genLoadRegister(Inst.RS, Func, RegType::Float);
        Value* RT  = genLoadRegister(Inst.RT, Func, RegType::Float);
        Value* Res = Builder->CreateFSub(RS, RT);
        genStoreRegister(Inst.RD, Res, Func, RegType::Float);
        break;
      }

    case dbt::OIDecoder::Subd: {
        Value* RS  = genLoadRegister(Inst.RS, Func, RegType::Double);
        Value* RT  = genLoadRegister(Inst.RT, Func, RegType::Double);
        Value* Res = Builder->CreateFSub(RS, RT);
        genStoreRegister(Inst.RD, Res, Func, RegType::Double);
        break;
      }

    case dbt::OIDecoder::Madds: {
        Value* RS  = genLoadRegister(Inst.RS, Func, RegType::Float);
        Value* RT  = genLoadRegister(Inst.RT, Func, RegType::Float);
        Value* RV  = genLoadRegister(Inst.RV, Func, RegType::Float);
        Value* Res1 = Builder->CreateFMul(RS, RT);
        Value* Res2 = Builder->CreateFAdd(Res1, RV);
        genStoreRegister(Inst.RD, Res2, Func, RegType::Float);
        break;
      }

    case dbt::OIDecoder::Maddd: {
        Value* RS  = genLoadRegister(Inst.RS, Func, RegType::Double);
        Value* RT  = genLoadRegister(Inst.RT, Func, RegType::Double);
        Value* RV  = genLoadRegister(Inst.RV, Func, RegType::Double);
        Value* Res1 = Builder->CreateFMul(RS, RT);
        Value* Res2 = Builder->CreateFAdd(Res1, RV);
        genStoreRegister(Inst.RD, Res2, Func, RegType::Double);
        break;
      }

    case dbt::OIDecoder::Msubs: {
        Value* RS  = genLoadRegister(Inst.RS, Func, RegType::Float);
        Value* RT  = genLoadRegister(Inst.RT, Func, RegType::Float);
        Value* RV  = genLoadRegister(Inst.RV, Func, RegType::Float);
        Value* Res1 = Builder->CreateFMul(RS, RT);
        Value* Res2 = Builder->CreateFSub(Res1, RV);
        genStoreRegister(Inst.RD, Res2, Func, RegType::Float);
        break;
      }

    case dbt::OIDecoder::Msubd: {
        Value* RS  = genLoadRegister(Inst.RS, Func, RegType::Double);
        Value* RT  = genLoadRegister(Inst.RT, Func, RegType::Double);
        Value* RV  = genLoadRegister(Inst.RV, Func, RegType::Double);
        Value* Res1 = Builder->CreateFMul(RS, RT);
        Value* Res2 = Builder->CreateFSub(Res1, RV);
        genStoreRegister(Inst.RD, Res2, Func, RegType::Double);
        break;
      }

    case dbt::OIDecoder::Mtc1: {
        Value* RS  = genLoadRegister(Inst.RS, Func);
        Value* Res = Builder->CreateBitCast(RS, Type::getFloatTy(TheContext));
        genStoreRegister(Inst.RT, Res, Func, RegType::Float);
        break;
      }

    case dbt::OIDecoder::Mfc1: {
        Value* RT  = genLoadRegister(Inst.RT, Func, RegType::Float);
        Value* Res = Builder->CreateBitCast(RT, Type::getInt32Ty(TheContext));
        genStoreRegister(Inst.RS, Res, Func, RegType::Int);
        break;
      }

    case dbt::OIDecoder::Mflc1: {
        Value* RT    = genLoadRegister(Inst.RT, Func, RegType::Double);
        Value* RTInt = Builder->CreateBitCast(RT, Type::getInt64Ty(TheContext));
        Value* Res   = Builder->CreateTrunc(Builder->CreateAnd(RTInt, 0xFFFFFFFF), Type::getInt32Ty(TheContext));
        genStoreRegister(Inst.RS, Res, Func, RegType::Int);
        break;
      }

    case dbt::OIDecoder::Mfhc1: {
        Value* RT    = genLoadRegister(Inst.RT, Func, RegType::Double);
        Value* RTInt = Builder->CreateBitCast(RT, Type::getInt64Ty(TheContext));
        Value* Res   = Builder->CreateTrunc(Builder->CreateLShr(RTInt, 32), Type::getInt32Ty(TheContext));
        genStoreRegister(Inst.RS, Res, Func, RegType::Int);
        break;
      }

    case dbt::OIDecoder::Mtlc1: {
        Value* RT    = genLoadRegister(Inst.RT, Func, RegType::Double);
        Value* RTInt = Builder->CreateBitCast(RT, Type::getInt64Ty(TheContext));
        Value* L     = Builder->CreateAnd(RTInt, 0xFFFFFFFF00000000ULL);
        Value* Res   = Builder->CreateAdd(L,
            Builder->CreateZExtOrBitCast(genLoadRegister(Inst.RS, Func), Type::getInt64Ty(TheContext)));
        Value* Res2  = Builder->CreateBitCast(Res, Type::getDoubleTy(TheContext));
        genStoreRegister(Inst.RT, Res2, Func, RegType::Double);
        break;
      }

    case dbt::OIDecoder::Mthc1: {
        Value* RT    = genLoadRegister(Inst.RT, Func, RegType::Double);
        Value* RTInt = Builder->CreateBitCast(RT, Type::getInt64Ty(TheContext));
        Value* L     = Builder->CreateAnd(RTInt, 0xFFFFFFFFULL);
        Value* Res   = Builder->CreateAdd(L,
            Builder->CreateShl(Builder->CreateZExtOrBitCast(genLoadRegister(Inst.RS, Func),
                Type::getInt64Ty(TheContext)), 32));
        Value* Res2  = Builder->CreateBitCast(Res, Type::getDoubleTy(TheContext));
        genStoreRegister(Inst.RT, Res2, Func, RegType::Double);
        break;
      }

    case dbt::OIDecoder::Truncwd: {
        Value* RT    = genLoadRegister(Inst.RT, Func, RegType::Double);
        Value* Int   = Builder->CreateFPToSI(RT, Type::getInt32Ty(TheContext));
        Value* Float = Builder->CreateBitCast(Int, Type::getFloatTy(TheContext));
        genStoreRegister(Inst.RS, Float, Func, RegType::Float);
        break;
      }

    case dbt::OIDecoder::Truncws: {
        Value* RT    = genLoadRegister(Inst.RT, Func, RegType::Float);
        Value* Int   = Builder->CreateFPToSI(RT, Type::getInt32Ty(TheContext));
        Value* Float = Builder->CreateBitCast(Int, Type::getFloatTy(TheContext));
        genStoreRegister(Inst.RS, Float, Func, RegType::Float);
        break;
      }

    case dbt::OIDecoder::Ldc1: {
        Value* RS    = genLoadRegister(Inst.RS, Func, RegType::Int);
        Value* Addrs = Builder->CreateAdd(RS, genImm(Inst.Imm));

        Value* Res = Builder->CreateLoad(genDataVecPtr(Addrs, Func, Type::getInt64Ty(TheContext), 8));
        genStoreRegister(Inst.RT, Res, Func, RegType::Int64);
        break;
      }

    case dbt::OIDecoder::Lwc1: {
        Value* RS    = genLoadRegister(Inst.RS, Func, RegType::Int);
        Value* Addrs = Builder->CreateAdd(RS, genImm(Inst.Imm));
        Value* Res   = Builder->CreateLoad(genDataWordVecPtr(Addrs, Func));
        Value* ResCasted = Builder->CreateBitCast(Res, Type::getFloatTy(TheContext));
        genStoreRegister(Inst.RT, ResCasted, Func, RegType::Float);
        break;
      }

    case dbt::OIDecoder::Lwxc1: {
        //M.setFloatRegister(I.RT, M.getMemValueAt(M.getRegister(I.RS) + I.Imm).asF_);
        Value* RS    = genLoadRegister(Inst.RS, Func, RegType::Int);
        Value* RT    = genLoadRegister(Inst.RT, Func, RegType::Int);
        Value* Addrs = Builder->CreateAdd(RS, RT);
        Value* Res   = Builder->CreateLoad(genDataWordVecPtr(Addrs, Func));
        Value* ResCasted = Builder->CreateBitCast(Res, Type::getFloatTy(TheContext));
        genStoreRegister(Inst.RD, ResCasted, Func, RegType::Float);
        break;
      }

    case dbt::OIDecoder::Ldxc1: {
        Value* RS    = genLoadRegister(Inst.RS, Func, RegType::Int);
        Value* RT    = genLoadRegister(Inst.RT, Func, RegType::Int);
        Value* RTS   = Builder->CreateAdd(RT, RS);

        Value* Res = Builder->CreateLoad(genDataVecPtr(RTS, Func, Type::getInt64Ty(TheContext), 8));
        genStoreRegister(Inst.RD, Res, Func, RegType::Int64);
        break;
      }

    case dbt::OIDecoder::Swc1: {
        Value* RS    = genLoadRegister(Inst.RS, Func, RegType::Int);
        Value* Addrs = Builder->CreateAdd(RS, genImm(Inst.Imm));

        Value* Res  = genLoadRegister(Inst.RT+66, Func, RegType::Int);

        Builder->CreateStore(Res, genDataWordVecPtr(Addrs, Func));
        break;
      }

    case dbt::OIDecoder::Swxc1: {
        Value* RS    = genLoadRegister(Inst.RS, Func, RegType::Int);
        Value* RT    = genLoadRegister(Inst.RT, Func, RegType::Int);
        Value* Addrs = Builder->CreateAdd(RS, RT);

        Value* Res  = genLoadRegister(Inst.RD+66, Func, RegType::Int);

        Builder->CreateStore(Res, genDataWordVecPtr(Addrs, Func));
        break;
      }

    case dbt::OIDecoder::Sdc1: {
        //RS+Immed
        Value* RS    = genLoadRegister(Inst.RS, Func, RegType::Int);
        Value* Addrs = Builder->CreateAdd(RS, genImm(Inst.Imm));

        Value* Res = genLoadRegister(Inst.RT, Func, RegType::Int64);
        Builder->CreateStore(Res, genDataVecPtr(Addrs, Func, Type::getInt64Ty(TheContext), 8));

        break;
      }

    case dbt::OIDecoder::Sdxc1: {
        Value* RS    = genLoadRegister(Inst.RS, Func, RegType::Int);
        Value* Addrs = Builder->CreateAdd(RS, genLoadRegister(Inst.RT, Func, RegType::Int));

        Value* Res = genLoadRegister(Inst.RD, Func, RegType::Int64);
        Builder->CreateStore(Res, genDataVecPtr(Addrs, Func, Type::getInt64Ty(TheContext), 8));

        break;
      }

    case dbt::OIDecoder::Sqrtd: {
        Value* RT  = genLoadRegister(Inst.RT, Func, RegType::Double);
        std::vector<Type *> arg_type;
        arg_type.push_back(Type::getDoubleTy(TheContext));
        Function *fun = Intrinsic::getDeclaration(Func->getParent(), Intrinsic::sqrt, arg_type);
        Value *Res = Builder->CreateCall(fun, {RT});
        genStoreRegister(Inst.RS, Res, Func, RegType::Double);
        break;
      }

    case dbt::OIDecoder::Sqrts: {
        Value* RT  = genLoadRegister(Inst.RT, Func, RegType::Float);
        std::vector<Type *> arg_type;
        arg_type.push_back(Type::getFloatTy(TheContext));
        Function *fun = Intrinsic::getDeclaration(Func->getParent(), Intrinsic::sqrt, arg_type);
        Value *Res = Builder->CreateCall(fun, {RT});
        genStoreRegister(Inst.RS, Res, Func, RegType::Float);
        break;
      }

    /************************************** JUMPS  ****************************************/

    case dbt::OIDecoder::Bc1t: {
        BasicBlock* BB = BasicBlock::Create(TheContext, "", Func);
        Value* Res = Builder->CreateICmpEQ(genLoadRegister(CC_REG, Func), genImm(1));
        BranchInst* Br = Builder->CreateCondBr(Res, BB, BB);
        Builder->SetInsertPoint(BB);
        IRBranchMap[GuestAddr] = Br;
        break;
      }

    case dbt::OIDecoder::Bc1f: {
        BasicBlock* BB = BasicBlock::Create(TheContext, "", Func);
        Value* Res = Builder->CreateICmpEQ(genLoadRegister(CC_REG, Func), genImm(0));
        BranchInst* Br = Builder->CreateCondBr(Res, BB, BB);
        Builder->SetInsertPoint(BB);
        IRBranchMap[GuestAddr] = Br;
        break;
      }

    case dbt::OIDecoder::Jeqz: {
        BasicBlock* BB = BasicBlock::Create(TheContext, "", Func);
        Value* Res = Builder->CreateICmpEQ(genLoadRegister(Inst.RS, Func), genImm(0));
        BranchInst* Br = Builder->CreateCondBr(Res, BB, BB);
        Builder->SetInsertPoint(BB);
        IRBranchMap[GuestAddr] = Br;
        setIfNotTheFirstInstGen(Br);
        break;
      }

    case dbt::OIDecoder::Jnez: {
        BasicBlock* BB = BasicBlock::Create(TheContext, "", Func);
        Value* Res = Builder->CreateICmpNE(genLoadRegister(Inst.RS, Func), genImm(0));
        BranchInst* Br = Builder->CreateCondBr(Res, BB, BB);
        Builder->SetInsertPoint(BB);
        IRBranchMap[GuestAddr] = Br;
        setIfNotTheFirstInstGen(Br);
        break;
      }

    case dbt::OIDecoder::Jeq: {
        BasicBlock* BB = BasicBlock::Create(TheContext, "", Func);
        Value* Res = Builder->CreateICmpEQ(genLoadRegister(Inst.RS, Func), genLoadRegister(Inst.RT, Func));
        BranchInst* Br = Builder->CreateCondBr(Res, BB, BB);
        Builder->SetInsertPoint(BB);
        IRBranchMap[GuestAddr] = Br;
        setIfNotTheFirstInstGen(Br);
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
        setIfNotTheFirstInstGen(Res);
        break;
      }

    case dbt::OIDecoder::Jgez: {
        BasicBlock* BB = BasicBlock::Create(TheContext, "", Func);
        Value* Res1 = Builder->CreateAnd(genLoadRegister(Inst.RT, Func), 0x80000000);
        Value* Res  = Builder->CreateICmpEQ(Res1, genImm(0));
        BranchInst* Br = Builder->CreateCondBr(Res, BB, BB);
        Builder->SetInsertPoint(BB);
        IRBranchMap[GuestAddr] = Br;
        setIfNotTheFirstInstGen(Res);
        break;
      }

    case dbt::OIDecoder::Jltz: {
        BasicBlock* BB = BasicBlock::Create(TheContext, "", Func);
        Value* Res1 = Builder->CreateAnd(genLoadRegister(Inst.RT, Func), 0x80000000);
        Value* Res  = Builder->CreateICmpNE(Res1, genImm(0));
        BranchInst* Br = Builder->CreateCondBr(Res, BB, BB);
        Builder->SetInsertPoint(BB);
        IRBranchMap[GuestAddr] = Br;
        setIfNotTheFirstInstGen(Res);
        break;
      }

    case dbt::OIDecoder::Jgtz: {
        BasicBlock* BB = BasicBlock::Create(TheContext, "", Func);
        Value* Res1 = Builder->CreateAnd(genLoadRegister(Inst.RT, Func), 0x80000000);
        Value* Res2 = Builder->CreateICmpNE(genLoadRegister(Inst.RT, Func), genImm(0));
        Value* Res  = genLogicalAnd(Builder->CreateNot(Res1), Res2, Func);
        BranchInst* Br = Builder->CreateCondBr(Res, BB, BB);
        Builder->SetInsertPoint(BB);
        IRBranchMap[GuestAddr] = Br;
        setIfNotTheFirstInstGen(Res);
        break;
      }

    case dbt::OIDecoder::Jne: {
        BasicBlock* BB = BasicBlock::Create(TheContext, "", Func);
        Value* Res = Builder->CreateICmpNE(genLoadRegister(Inst.RS, Func), genLoadRegister(Inst.RT, Func));
        BranchInst* Br = Builder->CreateCondBr(Res, BB, BB);
        Builder->SetInsertPoint(BB);
        IRBranchMap[GuestAddr] = Br;
        setIfNotTheFirstInstGen(Res);
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

    case dbt::OIDecoder::Jumpr: { 
        Value* S = Builder->CreateRet(genLoadRegister(Inst.RT, Func));
        setIfNotTheFirstInstGen(S);
        BasicBlock* BB = BasicBlock::Create(TheContext, "", Func);
        Builder->SetInsertPoint(BB);
        break;
      }

    case dbt::OIDecoder::Call: {
        auto GuestTarget = ((GuestAddr & 0xF0000000) | (Inst.Addrs << 2));
        Value* Res = genStoreRegister(31, genImm(GuestAddr + 4), Func);

        llvm::Function* Callee = Mod->getFunction("r"+std::to_string(GuestTarget));
        if (Callee) {
          //
          // nextAddr <- call
          // if nextAddr is call+4, jumps to there 
          // if not return
          //
          Value* NextAddr = Builder->CreateCall(Callee, {Func->arg_begin(), Func->arg_begin()+1, genImm(GuestTarget)});
          
          BasicBlock* AddrOk = BasicBlock::Create(TheContext, "CallOk", Func);
          BasicBlock* AddrWrong = BasicBlock::Create(TheContext, "CallWrong", Func);

          Value* CmpRes = Builder->CreateICmpEQ(genImm(GuestAddr + 4), NextAddr);
          Builder->CreateCondBr(CmpRes, AddrOk, AddrWrong);

          Builder->SetInsertPoint(AddrWrong);
          insertDirectExit(NextAddr);
          Builder->SetInsertPoint(AddrOk);
        } else {
          CallTargetList[GuestTarget].insert(GuestAddr);

          BasicBlock* BB = BasicBlock::Create(TheContext, "MissedCall", Func);
          BranchInst* Br;
        
          Br = Builder->CreateBr(BB);

          Builder->SetInsertPoint(BB);
          IRBranchMap[GuestAddr] = Br;
        }
        setIfNotTheFirstInstGen(Res);
        break;
    }

    case dbt::OIDecoder::Callr: { 
        Value* Res = genStoreRegister(31, genImm(GuestAddr + 4), Func);
        
        if (Trampoline != nullptr) {
          Builder->CreateStore(genLoadRegister(Inst.RT, Func), ReturnAddrs);
          Builder->CreateBr(Trampoline);
        } else {
          insertDirectExit(genLoadRegister(Inst.RT, Func));
        }

        BasicBlock* BB = BasicBlock::Create(TheContext, "", Func);
        Builder->SetInsertPoint(BB);
        setIfNotTheFirstInstGen(Res);
        break;
      }

    case dbt::OIDecoder::Ijmp: { 
        Value* IjmpReg  = genLoadRegister(IJMP_REG, Func);
        Value* IjmpReg2 = Builder->CreateAnd(IjmpReg, 0xFFFFF000);
        Value* IjmpReg3 = Builder->CreateOr(IjmpReg2, Inst.Imm & 0xFFF);
        genStoreRegister(IJMP_REG, IjmpReg3, Func);
        Value* IPointer = Builder->CreateAdd(IjmpReg3, genLoadRegister(Inst.RT, Func));
        Value* Target   = Builder->CreateLoad(genDataWordVecPtr(IPointer, Func));

        if (Trampoline != nullptr) {
          Builder->CreateStore(Target, ReturnAddrs);
          Builder->CreateBr(Trampoline);
        } else {
          insertDirectExit(Target);
        }

        BasicBlock* BB = BasicBlock::Create(TheContext, "", Func);
        Builder->SetInsertPoint(BB);
        setIfNotTheFirstInstGen(IjmpReg);
        break;
      }

    case dbt::OIDecoder::Syscall:{
        //syscallIR.generateSyscallIR(TheContext, Func, Builder, GuestAddr);
        Value* Res = insertDirectExit(genImm(GuestAddr));
        BasicBlock* BB = BasicBlock::Create(TheContext, "", Func);
        Builder->SetInsertPoint(BB);
        setIfNotTheFirstInstGen(Res);
        break;
      }

    default: {
        std::cerr << "Mother of God! We don't have support to emit inst at: " << std::hex << GuestAddr << " (" <<
          dbt::OIPrinter::getString(Inst) << ")\n";
        exit(1);
      }
  }
  static int inst_i = 0;

  if (!FirstInstGen) {
    std::cerr << "First Instruction not set for inst at " << std::hex << GuestAddr << "\n";
    exit(1);
  }

  addFirstInstToMap(GuestAddr);

  LastEmittedAddrs = GuestAddr;
  LastEmittedInst  = Inst;
}

void dbt::IREmitter::updateBranchTarget(uint32_t GuestAddr, std::array<uint32_t, 2> Tgts) {
  if (!IRBranchMap[GuestAddr]) return;

  Function* F = Builder->GetInsertBlock()->getParent();

  for (int i = 0; i < 2; i++) {
    uint32_t AddrTarget = Tgts[i];

    if (AddrTarget == 0)
      continue;

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
      insertDirectExit(genImm(AddrTarget));
    }
    IRBranchMap[GuestAddr]->setSuccessor(i, BBTarget);
  }
}

void dbt::IREmitter::processBranchesTargets(const OIInstList& OIRegion) {
  uint32_t NextAddrs = 0;
  for (unsigned I = 0; I < OIRegion.size(); I++) {
    auto Pair = OIRegion[I];
    if (I < OIRegion.size()-1)
      NextAddrs = OIRegion[I+1][0];
    else
      NextAddrs = 0;

    uint32_t GuestAddr = Pair[0];
    dbt::OIDecoder::OIInst Inst = dbt::OIDecoder::decode(Pair[1]);  

    if (OIDecoder::isControlFlowInst(Inst))
      updateBranchTarget(GuestAddr, OIDecoder::getPossibleTargets(GuestAddr, Inst));
  }
}

Function* dbt::IREmitter::createFunctionPrototype(uint32_t EntryAddress) {
  std::array<Type*, 3> ArgsType = {Type::getInt32PtrTy(TheContext), Type::getInt32PtrTy(TheContext), Type::getInt32Ty(TheContext)};
  FunctionType *FT = FunctionType::get(Type::getInt32Ty(TheContext), ArgsType, false);
  Function *F = cast<Function>(Mod->getOrInsertFunction("r" + std::to_string(EntryAddress), FT));

  F->addFnAttr(Attribute::ArgMemOnly);
  F->addFnAttr(Attribute::AlwaysInline);
  //F->setCallingConv(CallingConv::Fast);
  F->addAttribute(1, Attribute::NoAlias);
  F->addAttribute(1, Attribute::NoCapture);
  F->addAttribute(2, Attribute::NoAlias);
  F->addAttribute(2, Attribute::NoCapture);

  return F;
}

void dbt::IREmitter::generateFunctionIR(uint32_t EntryAddress, const OIInstList& OIRegion) {
  Trampoline = nullptr;
  LastEmittedAddrs = 0;
  IRBranchMap.clear();
  IRMemoryMap.clear();
  VolatileRegisters.clear();
  CallTargetList.clear();
  ReturnPoints.clear();
  CurrentEntryAddrs = EntryAddress;

  Function* F = Mod->getFunction("r"+std::to_string(EntryAddress));

  if (F == nullptr) {
    std::cerr << "Function being defined but not declared! " << EntryAddress << "\n";
    exit(1);
  }

  // Entry block to function must not have predecessors!
  RegionEntry = BasicBlock::Create(TheContext, "entry", F);
  RegionExit  = BasicBlock::Create(TheContext, "exit", F);
  BasicBlock *BB = BasicBlock::Create(TheContext, "", F);

  Builder->SetInsertPoint(RegionEntry);
  
  ReturnAddrs = Builder->CreateAlloca(Type::getInt32Ty(TheContext));

  Builder->SetInsertPoint(BB);

  // Listing all existing addresses. This enables look-ahead while emitting code.
  for (auto Pair : OIRegion)
    IRMemoryMap[Pair[0]] = nullptr;

  for (auto Pair : OIRegion) {
    dbt::OIDecoder::OIInst Inst = dbt::OIDecoder::decode(Pair[1]);  
    generateInstIR(Pair[0], Inst);
  }

  insertDirectExit(genImm(OIRegion.back()[0]+4));

  processBranchesTargets(OIRegion);

  Builder->SetInsertPoint(RegionEntry);
  Builder->CreateBr(BB);

  emmitExit(F);
}

void dbt::IREmitter::generateRegionIR(std::vector<uint32_t> EntryAddresses, OIInstList& OIRegion,
    uint32_t MemOffset, dbt::Machine& M, TargetMachine& TM, volatile uint64_t* NativeRegions, Module* TheModule) {
  
  Mod = TheModule;
  Mach = &M;
  CurrentNativeRegions = NativeRegions;
  DataMemOffset = MemOffset;

  TheModule->setDataLayout(TM.createDataLayout());
  TheModule->setTargetTriple(TM.getTargetTriple().str());

  std::sort(OIRegion.begin(), OIRegion.end(), [](const auto& lhs, const auto& rhs ) { return lhs[0] < rhs[0]; });

  OIInstList EarlyAddresses;
  for (auto I = OIRegion.begin(); I != OIRegion.end();) {
    if ((*I)[0] == EntryAddresses[0]) break;
    EarlyAddresses.push_back(*I);
    I = OIRegion.erase(I);
  }
  
  for (auto Pair : EarlyAddresses)
    OIRegion.push_back(Pair);

  std::vector<std::pair<uint32_t, OIInstList>> FunctionList;
  for (auto Pair : OIRegion) {
    if (M.isMethodEntry(Pair[0]) && Pair[0] != EntryAddresses[0]) {
      OIInstList R;
      for (auto P : OIRegion) 
        if (P[0] >= Pair[0] && P[0] < M.getMethodEnd(Pair[0])) {
          R.push_back(P);
        }
      FunctionList.push_back({Pair[0], R});
    }
  }

  // Remove repated instructions from the main region
  for (auto I = OIRegion.begin(); I != OIRegion.end();) {
    uint32_t Addrs = (*I)[0];
    bool isRepeated = false;
    for (auto J : FunctionList) {
      if (EntryAddresses[0] >= J.first && EntryAddresses[0] <= M.getMethodEnd(J.first)) continue;
      if (Addrs >= J.first && Addrs <= M.getMethodEnd(J.first)) {
        isRepeated = true;
        break;
      }
    }
    if (isRepeated)
      I = OIRegion.erase(I);
    else
      ++I;
  }

  // First create all the prototypes so we can know where we can jump/call
  createFunctionPrototype(EntryAddresses[0]);
  for (auto Func : FunctionList)  
    createFunctionPrototype(Func.first);

  // Generate a LLVM IR Function for each function scope inside the region
  for (auto Func : FunctionList)
    generateFunctionIR(Func.first, Func.second);

  // Emit the LLVM IR for the region
  generateFunctionIR(EntryAddresses[0], OIRegion);
}
