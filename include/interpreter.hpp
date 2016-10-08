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
    uint32_t LastStartAddrs, LastEndAddrs;
    int** DispatchValues;

    bool isAddrsContainedIn(uint32_t, uint32_t);

    void dispatch(Machine&, uint32_t, uint32_t);

    void* getDispatchValue(uint32_t);
    void setDispatchValue(uint32_t, int*);
  public:
    void execute(Machine&, uint32_t, uint32_t);

    ~ITDInterpreter() { free(DispatchValues); };
  };
}
