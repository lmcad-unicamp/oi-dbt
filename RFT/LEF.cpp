#include <RFT.hpp>

#include <memory>
#include <queue>

using namespace dbt;

void LEF::expand(uint32_t RecordingEntry, Machine& M) {
  llvm::errs() << "Trying to Expand!!!\n"; 
  std::set<uint32_t> Visited;
  std::queue<uint32_t> ToProcess;
  ToProcess.push(RecordingEntry);

  OIInstList ExpandedRegion;
  uint32_t ExpandedEntry = 0;
  while (!ToProcess.empty()) {
    uint32_t Entry = ToProcess.front();
    ToProcess.pop();

    OIInstList Region = TheManager.getCompiledOIRegion(Entry);
    ExpandedRegion.insert(ExpandedRegion.end(), Region.begin(), Region.end());

    if (CamesFromCall.count(Entry)) {
      ExpandedEntry = Entry;
      break;
    }

    for (auto Adjacent : TheManager.getDirectTransitions(Entry)) {
      if (Visited.count(Adjacent) == 0) {
        Visited.insert(Adjacent);
        ToProcess.push(Adjacent);
      }
    }
  }

  if (ExpandedEntry != 0 && ExpandedEntry != RecordingEntry) {
    llvm::errs() << "Expanding!!!\n"; 
    startRegionFormation(ExpandedEntry);
    OIRegion = ExpandedRegion;
    finishRegionFormation();
  }
}

void LEF::endRecording(Machine& M) {
  finishRegionFormation(); 
  if (hasRet) 
    expand(RecordingEntry, M);
  hasRet = false;
}

void LEF::onBranch(Machine& M) {
  if (Recording) { 
    for (uint32_t I = LastTarget; I <= M.getLastPC(); I += 4) {
      insertInstruction(I, M.getInstAt(I).asI_);
      OIDecoder::OIInstType InstType = OIDecoder::decode(M.getInstAt(I).asI_).Type;
      hasRet = hasRet || InstType == OIDecoder::OIInstType::Jumpr;
      if (InstType == OIDecoder::OIInstType::Call) // TODO: Should also test callr
        CamesFromCall[RecordingEntry] = true; 
    }
  }

  if (M.getPC() < M.getLastPC()) {
    if (!Recording) { 
      ++ExecFreq[M.getPC()];
      if (!TheManager.isRegionEntry(M.getPC()) && ExecFreq[M.getPC()] > HotnessThreshold) {
        startRegionFormation(M.getPC());
        if (OIDecoder::decode(M.getInstAt(M.getLastPC()).asI_).Type == OIDecoder::OIInstType::Call) // TODO: Should also test callr
          CamesFromCall[M.getPC()] = true; 
      }
    } else {
      endRecording(M);
    }
  }

  if (TheManager.isNativeRegionEntry(M.getPC())) {
    if (Recording) 
      endRecording(M);

    auto Next = TheManager.jumpToRegion(M.getPC(), M); 
    M.setPC(Next);

    ++ExecFreq[Next];
    if (ExecFreq[M.getPC()] > HotnessThreshold)
      startRegionFormation(Next);
  } 

  LastTarget = M.getPC();
}
