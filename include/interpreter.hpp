#include <machine.hpp>

namespace dbt {
  class Interpreter {
  public:
    virtual void execute(Machine&, uint32_t, uint32_t) = 0;

    void executeAll(Machine& M) {
      execute(M, M.getCodeStartAddrs(), M.getCodeEndAddrs());
    }
  };

  class ITDInterpreter : public Interpreter {
  private:
    struct DecodedInst {
      uint8_t RS, RT, RD, RV;
      int16_t Imm;
      uint32_t Addrs;
    };

    uint32_t LastStartAddrs, LastEndAddrs;
    int** DispatchValues;
    DecodedInst* DecodedInsts;

    bool isAddrsContainedIn(uint32_t, uint32_t);

    void dispatch(Machine&, uint32_t, uint32_t);

    void* getDispatchValue(uint32_t);
    void setDispatchValue(uint32_t, int*);

    DecodedInst getDecodedInst(uint32_t);
    void setDecodedInst(uint32_t, DecodedInst);
  public:
    void execute(Machine&, uint32_t, uint32_t);

    ~ITDInterpreter() { free(DispatchValues); };
  };
}
