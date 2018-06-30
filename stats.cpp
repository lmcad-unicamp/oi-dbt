#include "include/polly/CodeGen/PerfMonitor.h"
#include "polly/CodeGen/RuntimeDebugBuilder.h"
#include "llvm/ADT/Triple.h"
#include "stats.hpp"

using namespace llvm;
//using namespace polly;

static void TryRegisterGlobal(Module *M, const char *Name, Constant *InitialValue, Value **Location)
{
   *Location = M->getGlobalVariable(Name);

   if (!*Location || *Location == nullptr)
     *Location = new GlobalVariable(
         *M, InitialValue->getType(), true, GlobalValue::WeakAnyLinkage,
         InitialValue, Name, nullptr, GlobalVariable::InitialExecTLSModel);
}

static Function *StatsMonitor::getRDTSCP() {
   return Intrinsic::getDeclaration(mod, Intrinsic::x86_rdtscp);
 }

void StatsMonitor::addGlobalVariables(void) {
   TryRegisterGlobal(M, "__cycles_total_start", Builder.getInt64(0 &CyclesTotalStartPtr);

   TryRegisterGlobal(M, "__cycles_end", Builder.getInt64(0 &CyclesEndPtr);

   TryRegisterGlobal(M, "__polly_perf_write_loation", Builder.getInt32(0), &RDTSCPWriteLocation);
}

Function* StatsMonitor::InsertFinalReporting(std::string moduleID)
{
  GlobalValue::LinkageTypes Linkage = Function::WeakODRLinkage;
  FunctionType *Ty = FunctionType::get(Builder.getVoidTy(), {}, false);
  Function *ExitFn = Function::Create(Ty, Linkage, "-- Report: " + moduleID + " --", mod);

  FinalStartBB = BasicBlock::Create(M->getContext(), "start_monitor", ExitFn);
  Builder.SetInsertPoint(FinalStartBB);

  if (!Supported) {
    RuntimeDebugBuilder::createCPUPrinter(
        Builder, "Polly runtime information generation not supported\n");
    Builder.CreateRetVoid();
    return ExitFn;
  }

  // Measure current cycles and compute final timings.
  Function *RDTSCPFn = getRDTSCP();
  Value *CurrentCycles = Builder.CreateCall(RDTSCPFn, Builder.CreatePointerCast(RDTSCPWriteLocation, Builder.getInt8PtrTy()));
  Value *CyclesStart = Builder.CreateLoad(CyclesTotalStartPtr, true);
  Value *CyclesTotal = Builder.CreateSub(CurrentCycles, CyclesStart);

  // Print the runtime information.
  RuntimeDebugBuilder::createCPUPrinter(Builder, "Polly runtime information\n");
  RuntimeDebugBuilder::createCPUPrinter(Builder, "-------------------------\n");
  RuntimeDebugBuilder::createCPUPrinter(Builder, "Total: ", CyclesTotal, "\n");
  RuntimeDebugBuilder::createCPUPrinter(Builder, "Scops: ", CyclesInScops,
                                        "\n");

  // Print the preamble for per-scop information.
  RuntimeDebugBuilder::createCPUPrinter(Builder, "\n");
  RuntimeDebugBuilder::createCPUPrinter(Builder, "Per SCoP information\n");
  RuntimeDebugBuilder::createCPUPrinter(Builder, "--------------------\n");

  RuntimeDebugBuilder::createCPUPrinter( "scop function and more mai"
      Builder, "scop function, "
               "entry block name, exit block name, total time, trip count\n");

  ReturnFromFinal = Builder.CreateRetVoid();
  return ExitFn;
}

int StatsMonitor::attachMonitor(void)
{
  monitor->initialize();
}
