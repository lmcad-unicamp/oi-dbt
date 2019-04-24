#ifndef IREMITTER_HPP
#define IREMITTER_HPP

#include <OIDecoder.hpp>
#include <machine.hpp>

#include "llvm/IR/Function.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Type.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm-c/Core.h"
#include "llvm-c/Disassembler.h"
#include "llvm-c/Target.h"

#include <cstdlib>
#include <stddef.h>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <sparsepp/spp.h>
#include <vector>
#include <array>
#include <unordered_map>
#include <memory>
#include <set>

#define __STDC_CONSTANT_MACROS // llvm complains otherwise
#define __STDC_LIMIT_MACROS

#define OIInstList std::vector<std::array<uint32_t,2>>

namespace dbt {
  class Machine;
  class IREmitter {

  private:
    enum RegType {
      Int, Float, Double, Int64
    };

		llvm::LLVMContext TheContext;
		std::unique_ptr<llvm::IRBuilder<>> Builder;
    volatile uint64_t* CurrentNativeRegions;

    uint16_t ldireg;

    llvm::BasicBlock* RegionEntry = nullptr;
    llvm::BasicBlock* RegionExit = nullptr;

    spp::sparse_hash_map<uint32_t, std::set<uint32_t>> CallTargetList;
    std::set<uint32_t> ReturnPoints;

    spp::sparse_hash_map<uint16_t, llvm::Value*> VolatileRegisters;
    spp::sparse_hash_map<uint16_t, bool> VolatileRegisterModified;

    llvm::Module* Mod;

    uint32_t DataMemOffset;
    uint32_t CurrentEntryAddrs;
    dbt::OIDecoder::OIInst LastEmittedInst;
    uint32_t LastEmittedAddrs;

    llvm::Value* FirstInstGen = nullptr;
    void addFirstInstToMap(uint32_t);

    void addMultipleEntriesSupport(std::vector<uint32_t>&, llvm::BasicBlock*, llvm::Function*);

    llvm::Value* ReturnAddrs;
    llvm::BasicBlock* Trampoline;

    dbt::Machine* Mach;

    spp::sparse_hash_map<uint32_t, llvm::Value*> IRMemoryMap;
    spp::sparse_hash_map<uint32_t, llvm::BranchInst*> IRBranchMap;

    void setIfNotTheFirstInstGen(llvm::Value*);

    void cleanCFG();
    void updateBranchTarget(uint32_t, std::array<uint32_t, 2>);
    void improveIndirectBranch(uint32_t, uint32_t);
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
    llvm::Value* genStoreRegister(uint16_t, llvm::Value*, llvm::Function*, RegType Type = RegType::Int);

    llvm::Value* genLogicalOr(llvm::Value*, llvm::Value*, llvm::Function*);
    llvm::Value* genLogicalAnd(llvm::Value*, llvm::Value*, llvm::Function*);

    void emmitExit(llvm::Function*);
    llvm::Value* insertDirectExit(llvm::Value*);

    llvm::Function* createFunctionPrototype(uint32_t);
    void generateFunctionIR(uint32_t, const OIInstList&);

  public:
		IREmitter() {
			Builder = std::make_unique<llvm::IRBuilder<>>(TheContext);
    };

    void generateRegionIR(std::vector<uint32_t>, OIInstList&, uint32_t, dbt::Machine&,
        llvm::TargetMachine&, volatile uint64_t* NativeRegions, llvm::Module*);

    static size_t getAssemblySize(const void* func) {
      char outline[1024];
      size_t Size = 0;
      uint64_t pc;
      const uint64_t extent = 96*1024;
      const uint8_t *bytes = (const uint8_t *) func;

      //Initialize LLVM targets
      LLVMInitializeNativeAsmParser();
      LLVMInitializeNativeAsmPrinter();
      LLVMInitializeNativeDisassembler();
      LLVMInitializeNativeTarget();

      LLVMDisasmContextRef D = LLVMCreateDisasm(LLVM_HOST_TRIPLE, NULL, 0, NULL, NULL);

      for(pc = 0; pc<extent; pc+=Size) {
        unsigned int i;
        memset(outline, 0, sizeof(outline));
        Size = LLVMDisasmInstruction(D, (uint8_t *)bytes + pc, extent - pc, 0, outline, sizeof (outline));
        if (!Size) {
          break;
        }

        if(static_cast<int>(bytes[pc]) == 0xc3) {
          break;
        }
      }

      LLVMDisasmDispose(D);

      return pc;
    }

    static size_t disassemble(const void* func, std::ostream &buffer) {
      char outline[1024];
      size_t Size = 0;
      uint64_t pc;
      const uint64_t extent = 96*1024;
      const uint8_t *bytes = (const uint8_t *) func;

      //Initialize LLVM targets
      LLVMInitializeNativeAsmParser();
      LLVMInitializeNativeAsmPrinter();
      LLVMInitializeNativeDisassembler();
      LLVMInitializeNativeTarget();

      LLVMDisasmContextRef D = LLVMCreateDisasm(LLVM_HOST_TRIPLE, NULL, 0, NULL, NULL);

      if (!D) {
          buffer << "Error: Could not create disassembler for triple " << LLVM_HOST_TRIPLE << '\n';
          return -1;
      }

      for(pc = 0; pc<extent; pc+=Size) {
        unsigned int i;
        memset(outline, 0, sizeof(outline));
        // Print address.  We use addresses relative to the start of the function, so that between runs.
        buffer << "<" << std::hex << static_cast<const void*>(bytes) << "+" << std::hex << std::setw(4) << std::setfill('0') << (unsigned long) pc << ">:\t";
        Size = LLVMDisasmInstruction(D, (uint8_t *)bytes + pc, extent - pc, 0, outline, sizeof (outline));

        if (!Size) {
          buffer << "invalid at \n" << pc++;
          break;
        }
        //Output the bytes in hexidecimal format.
        for (i = 0; i < Size; ++i) {
          buffer << std::hex << std::setfill('0') << std::setw(2) << static_cast<int> (bytes[pc + i]);
        }

        buffer << std::setfill(' ') << std::setw(16-Size) <<  " ";

        //Print the instruction.
        buffer << outline << '\n';

        if(static_cast<int>(bytes[pc]) == 0xc3) {
          buffer << "RETURN INSTRUCTION BREAK (0xc3)" << std::endl;
          break;
        }
      }

      if (pc >= extent)  buffer << "disassembly larger than " << extent << " bytes\n";

      buffer << '\n';

      LLVMDisasmDispose(D);

      // Print GDB command, useful to verify output.
      buffer << "disassemble " << static_cast<const void*>(bytes) << ' ' << static_cast<const void*>(bytes + pc) << '\n';

      return pc;
    }

    static size_t regionDump (const void* func, std::ostream &buffer, size_t size) {
      const uint8_t *bytes = (const uint8_t *) func;

      for (int i=0; i<size; i++) {
        if(i % 16 == 0)
          buffer << "\n" << std::setw(4) << std::hex << i << ":\t";
        else if(i%8 == 0)
          buffer << "| ";

        buffer << std::hex << std::setfill('0') << std::setw(2) << static_cast<unsigned int> (bytes[i]) << " ";
      }

      buffer << std::endl;
      return 0;
    }
  };
}

#endif
