#ifndef IREMITTER_HPP
#define IREMITTER_HPP

#include <OIDecoder.hpp>

#include <llvm/IR/Function.h>

#include <vector>

#define OIInstList std::vector<uint32_t>

namespace dbt {
  class IREmitter {
  private:
    llvm::Value* generateInstIR(const OIDecoder::OIInst);
  public:
    llvm::Function* generateRegionIR(const OIInstList);
  };
}

#endif
