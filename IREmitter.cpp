#include <IREmitter.hpp>

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Type.h"

#include <cstdlib>

using namespace llvm;

Value* genLoadRegister(uint8_t RegNum) {
  return nullptr;
}

Value* genImm(uint16_t Imm) {
  return nullptr;
}

Value* dbt::IREmitter::generateInstIR(const dbt::OIDecoder::OIInst Inst) {
  switch (Inst.Type) {
  case dbt::OIDecoder::Addi:
    return Builder->CreateAdd(genLoadRegister(Inst.RS), genImm(Inst.Imm));
  default:
    return nullptr;
  }
}

StringRef dbt::IREmitter::generateUniqueName(StringRef Prefix) {
  while (true) {
    Twine PossibleName = Prefix + std::to_string(rand() % 1000);
    if (NamedValues.count(PossibleName.str()) == 0) 
      return PossibleName.str();
  }
}

Function* dbt::IREmitter::generateRegionIR(const OIInstList OIRegion) {
  FunctionType *FT = FunctionType::get(Type::getInt64Ty(TheContext), false);
  StringRef NewRegionName = generateUniqueName("Region");
  Function *F = Function::Create(FT, Function::ExternalLinkage, NewRegionName, TheModule.get());

  for (uint32_t RawInst : OIRegion) {
    OIDecoder::OIInst Inst = OIDecoder::decode(RawInst);
    Value* V = generateInstIR(Inst);
  }
  return F;
}
