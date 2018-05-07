#ifndef SYSCALLIREMITTER_HPP
#define SYSCALLIREMITTER_HPP

#include <OIDecoder.hpp>
#include <syscall.hpp>
#include <IREmitter.hpp>
#include <map>

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


namespace dbt {
  class SyscallIREmitter
  {
  private:
    IREmitter& emitter;
    void generateSyscallGet(const int syscallNo);
    static std::map<std::string, llvm::Function*> external_functions_;

  public:
    SyscallIREmitter(IREmitter& E) : emitter(E) {};
    void generateSyscallIR(llvm::LLVMContext& TheContext, llvm::Function* Func, std::unique_ptr<llvm::IRBuilder<>>& Builder, const uint32_t GuestAddr);
  };
}

#endif
