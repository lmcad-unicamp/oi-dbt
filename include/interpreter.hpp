#include <OIDecoder.hpp>
#include <machine.hpp>
#include <syscall.hpp>


namespace dbt {
  class Interpreter {
  protected:
    SyscallManager& SyscallM;

  public:
    Interpreter(SyscallManager& SM) : SyscallM(SM) {}

    virtual void execute(Machine&, uint32_t, uint32_t) = 0;

    void executeAll(Machine& M) {
      execute(M, M.getCodeStartAddrs(), M.getCodeEndAddrs());
    }
  };

  class ITDInterpreter : public Interpreter {
  private:
    RFT& ImplRFT;

    uint32_t LastStartAddrs, LastEndAddrs;
    std::vector<int*> DispatchValues;
    std::vector<OIDecoder::OIInst> DecodedInsts;

    bool isAddrsContainedIn(uint32_t, uint32_t);

    void dispatch(Machine&, uint32_t, uint32_t);

    void* getDispatchValue(uint32_t);
    void setDispatchValue(uint32_t, int*);

    OIDecoder::OIInst getDecodedInst(uint32_t);
    void setDecodedInst(uint32_t, OIDecoder::OIInst);
  public:
    ITDInterpreter(SyscallManager& SM, RFT& R) : Interpreter(SM), ImplRFT(R) {}

    void execute(Machine&, uint32_t, uint32_t);
  };
}
