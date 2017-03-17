//===-- elfLoader.hpp - Machine class definition -------*- C++ -*-===//
//
//                     The OpenISA Infrastructure
//
// This file is distributed under the MIT License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the Machine class.
///
//===----------------------------------------------------------------------===//
#ifndef MACHINE_HPP
#define MACHINE_HPP

#include <RFT.hpp>

#include <cstdint>
#include <unordered_map>
#include <memory>
#include <vector>
#include <functional>

#define uptr std::unique_ptr

namespace dbt {
  union Word {
    char asC_[4];
    uint32_t asI_;
  };

  class RFT;

  class Machine {
  private:
    int32_t Register[64];
    double DoubleRegister[64];
    uptr<char[]> DataMemory;
    uptr<Word[]> CodeMemory;

    uint32_t DataMemOffset;
    uint32_t CodeMemOffset;

    uint32_t DataMemLimit;
    uint32_t CodeMemLimit;

    uint32_t PC;

    RFT& ImplRFT;
  public:
    Machine(RFT& R) : ImplRFT(R) { Register[0] = 0; };

    void setCodeMemory(uint32_t, uint32_t, const char*);
    void setDataMemory(uint32_t, uint32_t, const char*);

    uint32_t getPC();
    void setPC(uint32_t);
    void incPC();

    uint32_t getSP();
    void setSP(uint32_t);

    Word getInstAt(uint32_t);
    Word getInstAtPC();
    Word getNextInst();

    Word getMemValueAt(uint32_t);
    void setMemValueAt(uint32_t, uint32_t);

    uint32_t getNumInst();
    uint32_t getCodeStartAddrs();
    uint32_t getCodeEndAddrs();
    uint32_t getDataMemOffset();

    int32_t getRegister(uint8_t);
    void setRegister(uint8_t, int32_t);

    uint32_t getDoubleRegister(uint8_t);
    void setDoubleRegister(uint8_t, double);

    int loadELF(const std::string);
  };
}

#endif
