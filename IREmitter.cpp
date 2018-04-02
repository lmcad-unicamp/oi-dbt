#include <IREmitter.hpp>
#include <OIPrinter.hpp>

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/ExecutionEngine/MCJIT.h"
#include "llvm/IR/CFG.h"
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

    //TODO
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

        // if (Inst.RV != 0) {
        //     Value* ResMod = Builder->CreateSRem(genLoadRegister(Inst.RS, Func), genLoadRegister(Inst.RT, Func));
        //     genStoreRegister(Inst.RV, ResMod, Func);
        // }
        
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
        break;
      }

    case dbt::OIDecoder::Xor: {
        Value* Res = Builder->CreateXor(genLoadRegister(Inst.RS, Func), genLoadRegister(Inst.RT, Func));
        genStoreRegister(Inst.RD, Res, Func);
        break;
      }

    case dbt::OIDecoder::Or: {
        Value* Res = Builder->CreateOr(genLoadRegister(Inst.RS, Func), genLoadRegister(Inst.RT, Func));
        genStoreRegister(Inst.RD, Res, Func);
        break;
      }

    case dbt::OIDecoder::Nor: {
        Value* Res = Builder->CreateOr(genLoadRegister(Inst.RS, Func), genLoadRegister(Inst.RT, Func));
        genStoreRegister(Inst.RD, Builder->CreateNot(Res), Func);
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
        Value* Res = Builder->CreateLoad(genDataWordVecPtr(RawAddrs, Func));
        genStoreRegister(Inst.RT, Res, Func);
        break;
      }

    case dbt::OIDecoder::Ldh: {
        Value* RawAddrs = Builder->CreateAdd(genLoadRegister(Inst.RS, Func), genImm(Inst.Imm));
        Value* Res = Builder->CreateLoad(genDataHalfVecPtr(RawAddrs, Func));
        genStoreRegister(Inst.RT, Builder->CreateIntCast(Res, Type::getInt32Ty(TheContext), true), Func);
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

    case dbt::OIDecoder::Movd: {  
        Value* Value = genLoadRegister(Inst.RT, Func, RegType::Double);
        genStoreRegister(Inst.RS, Value, Func, RegType::Double);
        break;
      } 

    case dbt::OIDecoder::Movs: {  
        Value* Value = genLoadRegister(Inst.RT, Func, RegType::Float);
        genStoreRegister(Inst.RS, Value, Func, RegType::Float);
        break;
      } 

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

    case dbt::OIDecoder::Cvtds: {  
        Value* FloatValue  = genLoadRegister(Inst.RT, Func, RegType::Float);
        Value* DoubleValue = Builder->CreateFPExt(FloatValue, Type::getDoubleTy(TheContext));
        genStoreRegister(Inst.RS, DoubleValue, Func, RegType::Double);
        break;
      } 

    case dbt::OIDecoder::Cvtdw: {  
        Value* FloatValue  = genLoadRegister(Inst.RT, Func, RegType::Float);
        Value* IntValue = Builder->CreateBitCast(FloatValue, Type::getInt32Ty(TheContext));
        Value* DoubleValue = Builder->CreateSIToFP(IntValue, Type::getDoubleTy(TheContext));
        genStoreRegister(Inst.RS, DoubleValue, Func, RegType::Double);
        break;
      } 

    case dbt::OIDecoder::Cvtsw: {  
        Value* RT  = genLoadRegister(Inst.RT, Func, RegType::Float);
        Value* Int = Builder->CreateBitCast(RT, Type::getInt32Ty(TheContext));
        Value* Float = Builder->CreateSIToFP(Int, Type::getFloatTy(TheContext));
        genStoreRegister(Inst.RS, Float, Func, RegType::Float);
        break;
      } 

    case dbt::OIDecoder::Cvtsd: {  
        Value* RT  = genLoadRegister(Inst.RT, Func, RegType::Double);
        Value* Float = Builder->CreateFPCast(RT, Type::getFloatTy(TheContext));
        genStoreRegister(Inst.RS, Float, Func, RegType::Float);
        break;
      } 

     case dbt::OIDecoder::Ceqd: {  
        Value* A = genLoadRegister(Inst.RS, Func, RegType::Double);
        Value* B = genLoadRegister(Inst.RT, Func, RegType::Double);
        Value* Res = Builder->CreateFCmpUEQ(A, B); 
        genStoreRegister(CC_REG, Builder->CreateZExt(Res, Type::getInt32Ty(TheContext)), Func, RegType::Int);
        break;
      }

     case dbt::OIDecoder::Ceqs: {  
        Value* A = genLoadRegister(Inst.RS, Func, RegType::Float);
        Value* B = genLoadRegister(Inst.RT, Func, RegType::Float);
        Value* Res = Builder->CreateFCmpUEQ(A, B);
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
        //M.setFloatRegister(I.RD, M.getFloatRegister(I.RS) + M.getFloatRegister(I.RT));
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

    case dbt::OIDecoder::Addd: {  
        //M.setDoubleRegister(I.RD, M.getDoubleRegister(I.RS) + M.getDoubleRegister(I.RT));
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
        Value* Res   = Builder->CreateAnd(RTInt, 0xFFFFFFFF);
        genStoreRegister(Inst.RS, Res, Func, RegType::Int);
        break;
      } 

    case dbt::OIDecoder::Mfhc1: {
        Value* RT    = genLoadRegister(Inst.RT, Func, RegType::Double);
        Value* RTInt = Builder->CreateBitCast(RT, Type::getInt64Ty(TheContext)); 
        Value* Res   = Builder->CreateLShr(RTInt, 32);
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
        Value* Reg = Builder->CreateAdd(genImm(Inst.RT*2), genImm(130));

        //M.setRegister(130 + I.RT*2 + 0, M.getMemValueAt(M.getRegister(I.RS) + I.Imm  ).asI_);
        Value* RS    = genLoadRegister(Inst.RS, Func, RegType::Int);
        Value* Addrs = Builder->CreateAdd(RS, genImm(Inst.Imm));
        Value* Res1  = Builder->CreateLoad(genDataWordVecPtr(Addrs, Func));
        genStoreRegister(Reg, Res1, Func, RegType::Int);

        //M.setRegister(130 + I.RT*2 + 1, M.getMemValueAt(M.getRegister(I.RS) + I.Imm + 4).asI_);
        Value* Addrs2 = Builder->CreateAdd(RS, genImm(Inst.Imm + 4));
        Value* Res2   = Builder->CreateLoad(genDataWordVecPtr(Addrs2, Func));
        Value* Reg2   = Builder->CreateAdd(Reg, genImm(1));
        genStoreRegister(Reg2, Res2, Func, RegType::Int);
        break;
      } 

    case dbt::OIDecoder::Lwc1: {
        //M.setFloatRegister(I.RT, M.getMemValueAt(M.getRegister(I.RS) + I.Imm).asF_);
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
        Value* T1  = Builder->CreateMul(genImm(Inst.RD), genImm(2));
        Value* Reg = Builder->CreateAdd(T1, genImm(130));

        //M.setRegister(130 + I.RT*2 + 0, M.getMemValueAt(M.getRegister(I.RS) + I.Imm  ).asI_);
        Value* RS    = genLoadRegister(Inst.RS, Func, RegType::Int);
        Value* Addrs = Builder->CreateAdd(RS, genLoadRegister(Inst.RT, Func, RegType::Int));
        Value* Res1  = Builder->CreateLoad(genDataWordVecPtr(Addrs, Func));
        genStoreRegister(Reg, Res1, Func, RegType::Int);

        //M.setRegister(130 + I.RT*2 + 1, M.getMemValueAt(M.getRegister(I.RS) + I.Imm + 4).asI_);
        Value* Addrs2 = Builder->CreateAdd(RS, genImm(Inst.Imm + 4));
        Value* Res2   = Builder->CreateLoad(genDataWordVecPtr(Addrs2, Func));
        Value* Reg2   = Builder->CreateAdd(Reg, genImm(1));
        genStoreRegister(Reg2, Res2, Func, RegType::Int);
        break;
      } 

    case dbt::OIDecoder::Swc1: {
        //M.setMemValueAt(M.getRegister(I.RS) + I.Imm, M.getRegister(66 + I.RT));
        Value* RS    = genLoadRegister(Inst.RS, Func, RegType::Int);
        Value* Addrs = Builder->CreateAdd(RS, genImm(Inst.Imm));

        Value* Reg = Builder->CreateAdd(genImm(Inst.RT), genImm(66));
        Value* Res  = genLoadRegister(Reg, Func, RegType::Int);

        Builder->CreateStore(Res, genDataWordVecPtr(Addrs, Func));
        break;
      } 

    case dbt::OIDecoder::Swxc1: {
        Value* RS    = genLoadRegister(Inst.RS, Func, RegType::Int);
        Value* RT    = genLoadRegister(Inst.RT, Func, RegType::Int);
        Value* Addrs = Builder->CreateAdd(RS, RT);

        Value* Reg = Builder->CreateAdd(genImm(Inst.RD), genImm(66));
        Value* Res  = genLoadRegister(Reg, Func, RegType::Int);

        Builder->CreateStore(Res, genDataWordVecPtr(Addrs, Func));
        break;
      } 

    case dbt::OIDecoder::Sdc1: {
        Value* RS    = genLoadRegister(Inst.RS, Func, RegType::Int);
        Value* Addrs = Builder->CreateAdd(RS, genImm(Inst.Imm));

        Value* T1  = Builder->CreateMul(genImm(Inst.RT), genImm(2));
        Value* Reg = Builder->CreateAdd(T1, genImm(130));
       
        //M.setMemValueAt(M.getRegister(I.RS) + I.Imm + 4, M.getRegister(130 + I.RT*2 + 1));
        Value* R1   = Builder->CreateAdd(Reg, genImm(1));
        Value* Res1 = genLoadRegister(R1, Func, RegType::Int);
        Value* Addrs1 = Builder->CreateAdd(Addrs, genImm(4));
        Builder->CreateStore(Res1, genDataWordVecPtr(Addrs1, Func));

        //M.setMemValueAt(M.getRegister(I.RS) + I.Imm, M.getRegister(130 + I.RT*2));
        Value* Res2 = genLoadRegister(Reg, Func, RegType::Int);
        Builder->CreateStore(Res2, genDataWordVecPtr(Addrs, Func));
        break;
      } 

    case dbt::OIDecoder::Sdxc1: {
        Value* RS    = genLoadRegister(Inst.RS, Func, RegType::Int);
        Value* Addrs = Builder->CreateAdd(RS, genLoadRegister(Inst.RT, Func, RegType::Int));

        Value* T1  = Builder->CreateMul(genImm(Inst.RD), genImm(2));
        Value* Reg = Builder->CreateAdd(T1, genImm(130));
       
        //M.setMemValueAt(M.getRegister(I.RS) + I.Imm + 4, M.getRegister(130 + I.RT*2 + 1));
        Value* R1   = Builder->CreateAdd(Reg, genImm(1));
        Value* Res1 = genLoadRegister(R1, Func, RegType::Int);
        Value* Addrs1 = Builder->CreateAdd(Addrs, genImm(4));
        Builder->CreateStore(Res1, genDataWordVecPtr(Addrs1, Func));

        //M.setMemValueAt(M.getRegister(I.RS) + I.Imm, M.getRegister(130 + I.RT*2));
        Value* Res2 = genLoadRegister(Reg, Func, RegType::Int);
        Builder->CreateStore(Res2, genDataWordVecPtr(Addrs, Func));
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

    case dbt::OIDecoder::Jgez: {
        BasicBlock* BB = BasicBlock::Create(TheContext, "", Func);
        Value* Res1 = Builder->CreateAnd(genLoadRegister(Inst.RT, Func), 0x80000000);
        Value* Res2 = Builder->CreateNot(Res1); 
        Value* Res  = Builder->CreateICmpNE(Res2, genImm(0));
        BranchInst* Br = Builder->CreateCondBr(Res, BB, BB);
        Builder->SetInsertPoint(BB);
        IRBranchMap[GuestAddr] = Br;
        break;
      }

    case dbt::OIDecoder::Jltz: {
        BasicBlock* BB = BasicBlock::Create(TheContext, "", Func);
        Value* Res1 = Builder->CreateAnd(genLoadRegister(Inst.RT, Func), 0x80000000);
        Value* Res  = Builder->CreateICmpNE(Res1, genImm(0));
        BranchInst* Br = Builder->CreateCondBr(Res, BB, BB);
        Builder->SetInsertPoint(BB);
        IRBranchMap[GuestAddr] = Br;
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
        IRIBranchMap[GuestAddr] = Builder->CreateRet(genLoadRegister(Inst.RT, Func));
        BasicBlock* BB = BasicBlock::Create(TheContext, "", Func);
        Builder->SetInsertPoint(BB);
        break;
      }

    case dbt::OIDecoder::Call: {
        auto GuestTarget = ((GuestAddr & 0xF0000000) | (Inst.Addrs << 2));

        Value* Res = genStoreRegister(31, genImm(GuestAddr + 4), Func);

        Argument *ArgDataMemPtr1 = &*(Func->arg_begin());
        Value *CastedPtr1 = Builder->CreatePointerCast(ArgDataMemPtr1, Type::getIntNPtrTy(TheContext, 32));
        Argument *ArgDataMemPtr2 = &*(Func->arg_begin()+1);
        Value *CastedPtr2 = Builder->CreatePointerCast(ArgDataMemPtr2, Type::getIntNPtrTy(TheContext, 32));
        Argument *ArgDataMemPtr3 = &*(Func->arg_begin()+2);
        Value *CastedPtr3 = Builder->CreatePointerCast(ArgDataMemPtr3, Type::getIntNPtrTy(TheContext, 64));

        Value *GEP = Builder->CreateGEP(CastedPtr3, genImm(GuestTarget));
        Value *HostTarget = Builder->CreateLoad(GEP);
        Value *Cond = Builder->CreateICmpEQ(HostTarget, Builder->CreateZExt(genImm(0), Type::getInt64Ty(TheContext)));

        BasicBlock* T = BasicBlock::Create(TheContext, "CallNonNative", Func);
        BasicBlock* F = BasicBlock::Create(TheContext, "CallNative", Func);
        BasicBlock* BB = BasicBlock::Create(TheContext, "AfterCall", Func);
        Builder->CreateCondBr(Cond, T, F);

        Builder->SetInsertPoint(F);
        std::array<Type*, 3> ArgsType = {Type::getInt32PtrTy(TheContext), Type::getInt32PtrTy(TheContext), Type::getInt64PtrTy(TheContext)};
        FunctionType *FT = FunctionType::get(Type::getInt32Ty(TheContext), ArgsType, false); 
        Value* V = Builder->CreateIntToPtr(HostTarget, PointerType::get(FT,0));

        Value *Ret = Builder->CreateCall(V, {CastedPtr1, CastedPtr2, CastedPtr3});
        Value *Cond2 = Builder->CreateICmpEQ(genImm(GuestAddr+4), Ret);

        BasicBlock* T2 = BasicBlock::Create(TheContext, "RetRight", Func);
        BasicBlock* F2 = BasicBlock::Create(TheContext, "RetWrong", Func);
        Builder->CreateCondBr(Cond2, T2, F2);

        Builder->SetInsertPoint(T2);
        Builder->CreateBr(BB);
        Builder->SetInsertPoint(F2);
        Builder->CreateRet(Ret);

        Builder->SetInsertPoint(T);
        BranchInst* Br = Builder->CreateBr(BB);
        Builder->SetInsertPoint(BB);
        IRBranchMap[GuestAddr] = Br;

        setIfNotTheFirstInstGen(Res);
        break;
    }

    case dbt::OIDecoder::Callr: { // TODO: improve indirect Jump
        Value* Res = genStoreRegister(31, genImm(GuestAddr + 4), Func);
        IRIBranchMap[GuestAddr] = Builder->CreateRet(genLoadRegister(Inst.RT, Func));
        BasicBlock* BB = BasicBlock::Create(TheContext, "", Func);
        Builder->SetInsertPoint(BB);
        setIfNotTheFirstInstGen(Res);
        break;
      }

    case dbt::OIDecoder::Ijmp: { // TODO: improve indirect Jump
        Value* IjmpReg  = genLoadRegister(IJMP_REG, Func);
        Value* IjmpReg2 = Builder->CreateAnd(IjmpReg, 0xFFFFF000);
        Value* IjmpReg3 = Builder->CreateOr(IjmpReg2, Inst.Imm & 0xFFF);
        genStoreRegister(IJMP_REG, IjmpReg3, Func);
        Value* Target   = Builder->CreateLoad(genDataWordVecPtr(IjmpReg3, Func));
        IRIBranchMap[GuestAddr] = Builder->CreateRet(Target);
        BasicBlock* BB = BasicBlock::Create(TheContext, "", Func);
        Builder->SetInsertPoint(BB);
        setIfNotTheFirstInstGen(IjmpReg);
        break;
      }

    case dbt::OIDecoder::Syscall:{ 
        Value* Res = Builder->CreateRet(genImm(GuestAddr));
        BasicBlock* BB = BasicBlock::Create(TheContext, "", Func);
        Builder->SetInsertPoint(BB);
        setIfNotTheFirstInstGen(Res);
        break;
      }

    default: {
        std::cout << "Mother of God! We don't have support to emit inst at: " << std::hex << GuestAddr << " (" <<
          dbt::OIPrinter::getString(Inst) << ")\n";
        exit(1);
      }
  }
  static int inst_i = 0;

  for (auto I = inst_begin(Func), E = inst_end(Func); I != E; ++I)
    MDNode* N = MDNode::get(TheContext, MDString::get(TheContext, ""));

  addFirstInstToMap(GuestAddr);
}

void dbt::IREmitter::updateBranchTarget(uint32_t GuestAddr, std::array<uint32_t, 2> Tgts) {
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
      insertDirectExit(AddrTarget);
    }
    IRBranchMap[GuestAddr]->setSuccessor(i, BBTarget);
  }
}

void dbt::IREmitter::improveIndirectBranch(uint32_t GuestAddr) {
  if (IRMemoryMap.count(BrTargets[GuestAddr]) != 0) {
    Instruction* GuestInst   = cast<Instruction>(IRMemoryMap[GuestAddr]);
    Value*       TargetAddrs = IRIBranchMap[GuestAddr]->getReturnValue();  

    Function* F = Builder->GetInsertBlock()->getParent();

    BasicBlock* IfFalse = GuestInst->getParent()->splitBasicBlock(IRIBranchMap[GuestAddr]);
    BasicBlock* IfBB    = BasicBlock::Create(TheContext, "IfBB", F);
    BasicBlock* IfTrue  = BasicBlock::Create(TheContext, "IfTrue", F);

    IfFalse->getUniquePredecessor()->getTerminator()->setSuccessor(0, IfBB); 

    Builder->SetInsertPoint(IfBB);
    Value* Bol = Builder->CreateICmpEQ(genImm(BrTargets[GuestAddr]), TargetAddrs, "ibr");
    Builder->CreateCondBr(Bol, IfTrue, IfFalse);

    Builder->SetInsertPoint(IfTrue);
    auto TargetInst = cast<Instruction>(IRMemoryMap[BrTargets[GuestAddr]]);
    BasicBlock *TargetBB = TargetInst->getParent();
    BasicBlock* BBTarget;
    if (TargetBB->getFirstNonPHI() == TargetInst) 
      BBTarget = TargetBB;
    else {
      BBTarget = TargetBB->splitBasicBlock(TargetInst);
    }
    Builder->CreateBr(BBTarget); 
  }
}

void dbt::IREmitter::processBranchesTargets(const OIInstList& OIRegion) {
  for (auto Pair : OIRegion) {
    OIDecoder::OIInst Inst = OIDecoder::decode(Pair[1]);
    uint32_t GuestAddr = Pair[0];

    if (OIDecoder::isControlFlowInst(Inst))
      updateBranchTarget(GuestAddr, OIDecoder::getPossibleTargets(GuestAddr, Inst));

    if (OIDecoder::isIndirectBranch(Inst) && BrTargets.count(GuestAddr) != 0)
      improveIndirectBranch(GuestAddr);
  }
}

Module* dbt::IREmitter::generateRegionIR(uint32_t EntryAddress, const OIInstList& OIRegion, uint32_t MemOffset, spp::sparse_hash_map<uint32_t, uint32_t>& BT, TargetMachine& TM, volatile uint64_t* NativeRegions) {
  CurrentNativeRegions = NativeRegions;
  static unsigned int id = 0;
  Module* TheModule = new Module(std::to_string(id), TheContext);
  TheModule->setDataLayout(TM.createDataLayout());
  TheModule->setTargetTriple(TM.getTargetTriple().str());

  IRMemoryMap.clear();
  IRBranchMap.clear();

  BrTargets = BT;
  DataMemOffset     = MemOffset;
  CurrentEntryAddrs = EntryAddress;
  std::array<Type*, 3> ArgsType = {Type::getInt32PtrTy(TheContext), Type::getInt32PtrTy(TheContext), Type::getInt64PtrTy(TheContext)};
  FunctionType *FT = FunctionType::get(Type::getInt32Ty(TheContext), ArgsType, false);
  Function *F = Function::Create(FT, Function::ExternalLinkage, "r" + std::to_string(EntryAddress), TheModule);
  
  F->setCallingConv(CallingConv::Fast);
  F->addAttribute(1, Attribute::NoAlias);
  F->addAttribute(1, Attribute::NoCapture);
  F->addAttribute(2, Attribute::NoAlias);
  F->addAttribute(2, Attribute::NoCapture);
  //F->setName("Module_" + std::to_string(id) + "_entry_" + std::to_string((unsigned int) EntryAddress));
  
  // Entry block to function must not have predecessors!
  BasicBlock *Entry = BasicBlock::Create(TheContext, "entry", F);
  BasicBlock *BB    = BasicBlock::Create(TheContext, "", F);

  Builder->SetInsertPoint(Entry);
  Builder->CreateBr(BB);
  Builder->SetInsertPoint(BB);

  for (auto Pair : OIRegion) {
    OIDecoder::OIInst Inst = OIDecoder::decode(Pair[1]);
    generateInstIR(Pair[0], Inst);
  }
  insertDirectExit(OIRegion.back()[0]+4);
  
  processBranchesTargets(OIRegion);

  id++;
  return TheModule;
}

Module* dbt::IREmitter::generateMergedRegions(std::vector<OIInstList>& OIRegions, uint32_t MemOffset, spp::sparse_hash_map<uint32_t, uint32_t>& BT, TargetMachine& TM) {
  static unsigned int id = 0;
  Module* TheModule = new Module(std::to_string(id), TheContext);
  TheModule->setDataLayout(TM.createDataLayout());
  TheModule->setTargetTriple(TM.getTargetTriple().str());

  IRMemoryMap.clear();
  IRBranchMap.clear();

  BrTargets = BT;
  DataMemOffset     = MemOffset;
  // //CurrentEntryAddrs = EntryAddress;
  const auto initial = OIRegions.front().front();
  std::array<Type*, 3> ArgsType = {Type::getInt32PtrTy(TheContext), Type::getInt32PtrTy(TheContext), Type::getInt64PtrTy(TheContext)};
  FunctionType *FT = FunctionType::get(Type::getInt32Ty(TheContext), ArgsType, false);
  Function *F = Function::Create(FT, Function::ExternalLinkage, "r" + std::to_string(initial[0]), TheModule);
  
  F->setCallingConv(CallingConv::X86_FastCall);
  F->addAttribute(1, Attribute::NoAlias);
  F->addAttribute(1, Attribute::NoCapture);
  F->addAttribute(2, Attribute::NoAlias);
  F->addAttribute(2, Attribute::NoCapture);
  //F->setName("Module_" + std::to_string(id) + "_entry_" + std::to_string((unsigned int) EntryAddress));
  
  // // Entry block to function must not have predecessors!
  BasicBlock *Entry = BasicBlock::Create(TheContext, "entry" + std::to_string(initial[0]), F);
  Builder->SetInsertPoint(Entry);
  
  //Entry addr, Block
  std::map<uint32_t, BasicBlock*> usedAddresses;
  BasicBlock* LastBB = nullptr;
  Value* LastRes = nullptr;
  
  BasicBlock *FBB = BasicBlock::Create(TheContext, "End", F);
  
  for (int i = 0; i < OIRegions.size(); ++i) {
    auto region = OIRegions[i];
    BasicBlock *BB = BasicBlock::Create(TheContext, "", F);
    
    if(LastRes) {
        if(usedAddresses[region.front()[0]]) {
            Builder->CreateCondBr(LastRes, BB, FBB);
            //PC next PC
            LastRes = Builder->CreateICmpEQ(genImm(OIRegions[i-1].back()[0]+4), genImm(region.front()[0]));
            LastBB = BB;
            continue;
        }
        else {
            Builder->CreateCondBr(LastRes, BB, FBB);
        }
    } else {
        Builder->CreateBr(BB);
    }

    Builder->SetInsertPoint(BB);
    usedAddresses[region.front()[0]] = BB;
    LastBB = BB;
    
    for (auto Pair : region) {
      OIDecoder::OIInst Inst = OIDecoder::decode(Pair[1]);
      generateInstIR(Pair[0], Inst);
    }
    
    //Insert Compare if yes then:
    if (i > 0) {
      LastRes = Builder->CreateICmpEQ(genImm(OIRegions[i-1].back()[0]+4), genImm(region.front()[0]));
    } else {
      LastRes = Builder->CreateICmpEQ(genImm(OIRegions[i+1].front()[0]+4), genImm(region.back()[0]));
    }
  }

  Builder->CreateBr(FBB);
  Builder->SetInsertPoint(FBB);
  
  insertDirectExit(OIRegions.back().back()[0]+4);
  
  processBranchesTargets(OIRegions.back());
  id++;
  return TheModule;
}
