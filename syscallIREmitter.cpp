#include <syscallIREmitter.hpp>

#include <iostream>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <ostream>

#include "llvm/IR/Function.h"
#include "llvm/IR/TypeBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Type.h"

using namespace dbt;
using namespace llvm;

extern dbt::Machine M;

//Exit  *
//Read  **
//Write **
//Open  **
//Close **
//Creat **
//LSeek **
//Fstat **

/*static Function* exit_prototype(LLVMContext& C, Module* mod)
{
  std::array<Type*, 1> ArgsType = {Type::getInt32Ty(C)};
  FunctionType *FT = FunctionType::get(Type::getVoidTy(C), ArgsType, false);
  Function* func = cast<Function>(mod->getOrInsertFunction("exit", FT));

  func->setCallingConv(CallingConv::C);
  //func->addAttribute(1, Attribute::NoAlias);
  func->setLinkage(GlobalValue::ExternalLinkage);

  return func;
}

static Function* read_prototype(LLVMContext& C, Module* mod)
{
  std::array<Type*, 3> ArgsType =
    {Type::getInt32Ty(C), Type::getInt8PtrTy(C), Type::getInt64Ty(C)};
  FunctionType *FT = FunctionType::get(
    Type::getInt64Ty(C), ArgsType, false);

  Constant *F = mod->getOrInsertFunction("read", FT);
  Function* func = dyn_cast<Function>(F->stripPointerCasts());
  assert(func != NULL && "Read Func Error....");

  func->addAttribute(~0u, Attribute::ReadOnly);
  func->addAttribute(~0u, Attribute::NoUnwind);
  func->addAttribute(1, Attribute::NoCapture);
  func->addAttribute(2, Attribute::NoCapture);
  func->addAttribute(3, Attribute::NoCapture);
  func->setCallingConv(CallingConv::C);
  func->setLinkage(GlobalValue::ExternalLinkage);
  return func;
}

static Function* write_prototype(LLVMContext& C, Module* mod)
{
  std::array<Type*, 3> ArgsType =
    {Type::getInt32Ty(C), Type::getInt8PtrTy(C), Type::getInt64Ty(C)};
  FunctionType *FT = FunctionType::get(
    Type::getInt64Ty(C), ArgsType, false);

  Constant *F = mod->getOrInsertFunction("write", FT);
  Function* func = dyn_cast<Function>(F->stripPointerCasts());
  assert(func != NULL && "Write Func Error....");

  func->addAttribute(~0u, Attribute::ReadOnly);
  func->addAttribute(~0u, Attribute::NoUnwind);
  func->addAttribute(1, Attribute::NoCapture);
  func->addAttribute(2, Attribute::NoCapture);
  func->addAttribute(3, Attribute::NoCapture);
  func->setCallingConv(CallingConv::C);
  func->setLinkage(GlobalValue::ExternalLinkage);
  return func;
}

static Function* open_prototype(LLVMContext& C, Module* mod)
{
  std::array<Type*, 2> ArgsType = {Type::getInt8PtrTy(C), Type::getInt32Ty(C)};
  FunctionType *FT = FunctionType::get(Type::getInt32Ty(C), ArgsType, false);
  Function* func = cast<Function>(mod->getOrInsertFunction("open", FT));

  func->setCallingConv(CallingConv::C);
  func->addAttribute(1, Attribute::NoAlias);
  func->addAttribute(2, Attribute::NoAlias);
  func->setLinkage(GlobalValue::ExternalLinkage);

  return func;
}

static Function* close_prototype(LLVMContext& C, Module* mod)
{
  std::array<Type*, 1> ArgsType = {Type::getInt32Ty(C)};
  FunctionType *FT = FunctionType::get(Type::getInt32Ty(C), ArgsType, false);
  Function* func = cast<Function>(mod->getOrInsertFunction("close", FT));

  func->setCallingConv(CallingConv::C);
  func->addAttribute(1, Attribute::NoAlias);
  func->setLinkage(GlobalValue::ExternalLinkage);

  return func;
}

static Function* creat_prototype(LLVMContext& C, Module* mod)
{
  std::array<Type*, 2> ArgsType = {Type::getInt8PtrTy(C), Type::getInt32Ty(C)};
  FunctionType *FT = FunctionType::get(Type::getInt32Ty(C), ArgsType, false);
  Function* func = cast<Function>(mod->getOrInsertFunction("creat", FT));

  func->setCallingConv(CallingConv::C);
  func->addAttribute(1, Attribute::NoAlias);
  func->addAttribute(2, Attribute::NoAlias);
  func->setLinkage(GlobalValue::ExternalLinkage);

  return func;
}

static Function* lseek_prototype(LLVMContext& C, Module* mod)
{
  std::array<Type*, 3> ArgsType = {Type::getInt32Ty(C), Type::getInt32Ty(C), Type::getInt32Ty(C)};
  FunctionType *FT = FunctionType::get(Type::getInt32Ty(C), ArgsType, false);
  Function* func = cast<Function>(mod->getOrInsertFunction("lseek", FT));

  func->setCallingConv(CallingConv::C);
  func->addAttribute(1, Attribute::NoAlias);
  func->addAttribute(2, Attribute::NoAlias);
  func->setLinkage(GlobalValue::ExternalLinkage);

  return func;
}

static Function* fstat_prototype(LLVMContext& C, Module* mod)
{
  std::array<Type*, 2> ArgsType = {Type::getInt32Ty(C), StructType::get(C)};
  FunctionType *FT = FunctionType::get(Type::getInt32Ty(C), ArgsType, false);
  Function* func = cast<Function>(mod->getOrInsertFunction("fstat", FT));

  func->setCallingConv(CallingConv::C);
  func->addAttribute(1, Attribute::NoAlias);
  func->addAttribute(2, Attribute::NoAlias);
  func->setLinkage(GlobalValue::ExternalLinkage);

  return func;
}

static Value* CastToCStr(Value *V, std::unique_ptr<IRBuilder<>>& Builder) {
  return Builder->CreateBitCast(V, Builder->getInt8PtrTy(), "cstr");
}

void SyscallIREmitter::generateSyscallIR(LLVMContext& TheContext, Function* Func, std::unique_ptr<IRBuilder<>>& Builder, const uint32_t GuestAddr)
{
  Module* TheModule = Func->getParent();
  auto lastFuncInst = inst_end(Func);

  //Basic Blocks for system calls
  BasicBlock* BBwrite   = BasicBlock::Create(TheContext, "write_syscall", Func);
  BasicBlock* BBread    = BasicBlock::Create(TheContext, "read_syscall", Func);
  /*BasicBlock* BBopen    = BasicBlock::Create(TheContext, "open_syscall", Func);
  BasicBlock* BBclose   = BasicBlock::Create(TheContext, "close_syscall", Func);
  BasicBlock* BBcreat   = BasicBlock::Create(TheContext, "creat_syscall", Func);
  BasicBlock* BBfstat   = BasicBlock::Create(TheContext, "fstat_syscall", Func);
  BasicBlock* BBlseek   = BasicBlock::Create(TheContext, "lseek_syscall", Func);
  *//*BasicBlock* BBelse    = BasicBlock::Create(TheContext, "interpret_syscall", Func);
  BasicBlock* MergeBB   = BasicBlock::Create(TheContext, "continue_syscall", Func);


  //Default Parameters
  Value* SysTy              =   Builder->CreateSub(emitter.genLoadRegister(4, Func, emitter.RegType::Int), emitter.genImm(4000));
  Argument *ArgIntRegPtr    =   Func->arg_begin()+1;
  Value* CastedMemPtr       =   ArgIntRegPtr;
  Value* MemoryOffsetPtr    =   Builder->CreateGEP(CastedMemPtr, emitter.genLoadRegister(6, Func, emitter.RegType::Int));
  Value* MemoryOffsetPtr2   =   Builder->CreateGEP(CastedMemPtr, emitter.genLoadRegister(5, Func, emitter.RegType::Int));
  Value* R5                 =   emitter.genLoadRegister(5, Func, emitter.RegType::Int);
  Value* R6                 =   emitter.genLoadRegister(6, Func, emitter.RegType::Int);
  Value* R7                 =   emitter.genLoadRegister(7, Func, emitter.RegType::Int);

  //Condition Write
  Value* Res = Builder->CreateICmpEQ(SysTy, emitter.genImm(LinuxSyscallManager::SyscallType::Write));
  Builder->CreateCondBr(Res, BBwrite, BBread);
  emitter.setIfNotTheFirstInstGen(Res);


  //Write Syscall
  Builder->SetInsertPoint(BBwrite);
  {
    Function* write_func    =   write_prototype(TheContext, TheModule);
    Value* Ret              =   Builder->CreateCall(write_func, {R5, CastToCStr(MemoryOffsetPtr, Builder), R7});
    emitter.genStoreRegister(2, Ret, Func, emitter.RegType::Int);
    Builder->CreateBr(MergeBB);
  }

  //Read Syscall
  Builder->SetInsertPoint(BBread);
  {
    BasicBlock* BB      =  BasicBlock::Create(TheContext, "", Func);

    Res = Builder->CreateICmpEQ(SysTy, emitter.genImm(LinuxSyscallManager::SyscallType::Read));
    Builder->CreateCondBr(Res, BB, BBelse);

    Builder->SetInsertPoint(BB);
    Function* read_func     =   read_prototype(TheContext, TheModule);
    Value* Ret              =   Builder->CreateCall(read_func, {R5, CastToCStr(MemoryOffsetPtr, Builder), R7});
    emitter.genStoreRegister(2, Ret, Func, emitter.RegType::Int);
    //emitter.setIfNotTheFirstInstGen(Ret);

    Builder->CreateBr(MergeBB);
  }
/*
  Builder->SetInsertPoint(BBopen);
  {
    BasicBlock* BB      =  BasicBlock::Create(TheContext, "", Func);

    Res = Builder->CreateICmpEQ(SysTy, emitter.genImm(LinuxSyscallManager::SyscallType::Open));
    Builder->CreateCondBr(Res, BB, BBclose);

    Builder->SetInsertPoint(BB);
    Function* open_func  =  open_prototype(TheContext, TheModule);
    BasicBlock* BB1      =  BasicBlock::Create(TheContext, "", Func);
    BasicBlock* BB2      =  BasicBlock::Create(TheContext, "", Func);
    Value* Comp          =  Builder->CreateICmpULT(R6, emitter.genImm(3));
    Builder->CreateCondBr(Comp, BB1, BB2);

    //Open for read
    Builder->SetInsertPoint(BB1);
    Value* Ret          =  Builder->CreateCall(open_func, {MemoryOffsetPtr2, R6});
    emitter.genStoreRegister(2, Ret, Func, emitter.RegType::Int);
    Builder->CreateBr(MergeBB);

    Builder->SetInsertPoint(BB2);
    Value* ValueWrite   =  emitter.genLogicalOr(R6, emitter.genImm(O_CREAT), Func);
    Ret                 =  Builder->CreateCall(open_func, {MemoryOffsetPtr2, ValueWrite});
    emitter.genStoreRegister(2, Ret, Func, emitter.RegType::Int);
    Builder->CreateBr(MergeBB);
  }

  Builder->SetInsertPoint(BBclose);
  {
    BasicBlock* BB      =  BasicBlock::Create(TheContext, "", Func);
    Res = Builder->CreateICmpEQ(SysTy, emitter.genImm(LinuxSyscallManager::SyscallType::Close));
    Builder->CreateCondBr(Res, BB, BBcreat);

    Builder->SetInsertPoint(BB);
    Function* close_func    =   close_prototype(TheContext, TheModule);
    Value* Ret              =   Builder->CreateCall(close_func, {R5});
    emitter.genStoreRegister(2, Ret, Func, emitter.RegType::Int);
    Builder->CreateBr(MergeBB);
  }

  Builder->SetInsertPoint(BBcreat);
  {
    BasicBlock* BB      =  BasicBlock::Create(TheContext, "", Func);
    Res = Builder->CreateICmpEQ(SysTy, emitter.genImm(LinuxSyscallManager::SyscallType::Creat));
    Builder->CreateCondBr(Res, BB, BBfstat);

    Builder->SetInsertPoint(BB);
    Function* creat_func    =   creat_prototype(TheContext, TheModule);
    Value* Ret              =   Builder->CreateCall(creat_func, {MemoryOffsetPtr2, R6});
    emitter.genStoreRegister(2, Ret, Func, emitter.RegType::Int);
    Builder->CreateBr(MergeBB);
  }

  Builder->SetInsertPoint(BBfstat);
  {
    BasicBlock* BB      =  BasicBlock::Create(TheContext, "", Func);
    Res = Builder->CreateICmpEQ(SysTy, emitter.genImm(LinuxSyscallManager::SyscallType::Fstat));
    Builder->CreateCondBr(Res, BB, BBlseek);

    Builder->SetInsertPoint(BB);
    //Function* fstat_func    =   fstat_prototype(TheContext, TheModule);
    emitter.genStoreRegister(2, emitter.genImm(-1), Func, emitter.RegType::Int);
    Builder->CreateBr(MergeBB);
  }

  Builder->SetInsertPoint(BBlseek);
  {
    BasicBlock* BB      =  BasicBlock::Create(TheContext, "", Func);
    Res = Builder->CreateICmpEQ(SysTy, emitter.genImm(LinuxSyscallManager::SyscallType::Lseek));
    Builder->CreateCondBr(Res, BB, BBelse);

    Builder->SetInsertPoint(BB);
    Function* lseek_func    =   lseek_prototype(TheContext, TheModule);
    Value* Ret              =   Builder->CreateCall(lseek_func, {R5, R6, R7});
    emitter.genStoreRegister(2, Ret, Func, emitter.RegType::Int);
    Builder->CreateBr(MergeBB);
  }

  //Interpret Syscall
  Builder->SetInsertPoint(BBelse);
  {
    //setIfNotTheFirstInstGen(Res);
    std::cout << "Interpret syscall emit!\n";
    Value* Res = Builder->CreateRet(emitter.genImm(GuestAddr));
    //BasicBlock* BB = BasicBlock::Create(TheContext, "", Func);
    //Builder->SetInsertPoint(BB);
    //emitter.setIfNotTheFirstInstGen(Res);

    //Builder->CreateBr(MergeBB);
  }

  //End of block
  Builder->SetInsertPoint(MergeBB);
}*/
