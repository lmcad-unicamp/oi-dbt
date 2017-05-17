#include <RFT.hpp>

#include <memory>

using namespace dbt;

void MRET2::mergePhases() {
  uint32_t addr1, addr2;
  unsigned i = 0, j = 0;

  if (0 == RecordingBufferTmp1.size() || 0 == RecordingBufferTmp2.size()) 
    return;

  while (i < RecordingBufferTmp1.size() && j < RecordingBufferTmp2.size()) {
    addr1 = RecordingBufferTmp1[i][0];
    addr2 = RecordingBufferTmp2[j][0];

    if (addr1 == addr2) 
      insertInstruction(RecordingBufferTmp1[i]);
    else
      break;

    i++;
    j++;
  }
}

uint32_t MRET2::getStoredIndex(uint32_t addr) {
  for (int i = 0; i < 1000; i++) { 
    if (stored[i].size() > 0 && stored[i][0][0] == addr) return i;
  }
  return 1001;
}

uint32_t MRET2::getPhase(uint32_t addr) {
  if (phases.count(addr) == 0) phases[addr] = 1;
  return phases[addr];
}

void MRET2::finishPhase() {
  if (getPhase(RecordingEntry) == 1) {
    stored[stored_index] = RecordingBufferTmp1;
    RecordingBufferTmp1.clear();

    stored_index++;
    if (stored_index == 1000) stored_index = 0;

    phases[RecordingEntry] = 2;
    Recording = false;
  } else {
    RecordingBufferTmp2.clear();
    if (getStoredIndex(RecordingEntry) != 1001)  
      RecordingBufferTmp2 = stored[getStoredIndex(RecordingEntry)];
    else 
      RecordingBufferTmp2 = RecordingBufferTmp1;
    mergePhases();
    phases[RecordingEntry] = 1;
    finishRegionFormation();
  }
}

void MRET2::onBranch(Machine& M) {
  if (M.getPC() < M.getLastPC()) {
    if (!Recording) { 
      ++ExecFreq[M.getPC()];
      if (!TheManager.isRegionEntry(M.getPC()) && ExecFreq[M.getPC()] > HotnessThreshold/2) { 
        startRegionFormation(M.getPC());
        RecordingBufferTmp1.clear();
        if (getPhase(M.getPC()) == 1) 
          ExecFreq[M.getPC()] = 0;
      }
    } else {
      finishPhase();
    }
  }
}

void MRET2::onNextInst(Machine& M) {
  if (TheManager.isNativeRegionEntry(M.getPC())) {
    if (Recording) 
      finishPhase();

    auto Next = TheManager.jumpToRegion(M.getPC(), M); 
    M.setPC(Next);

    ++ExecFreq[Next];
    if (ExecFreq[M.getPC()] > HotnessThreshold)
      startRegionFormation(Next);
  } else {
    if (Recording) 
      RecordingBufferTmp1.push_back({M.getPC(), M.getInstAtPC().asI_});
  }
}
