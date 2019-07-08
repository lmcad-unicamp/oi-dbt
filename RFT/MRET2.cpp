#include <RFT.hpp>

#include <memory>

using namespace dbt;

void MRET2::mergePhases() {
  uint32_t addr1, addr2;
  unsigned i = 0;

  if (RecordingBufferTmp1.size() == 0 || RecordingBufferTmp2.size() == 0) 
    return;

  while (i < RecordingBufferTmp1.size() && i < RecordingBufferTmp2.size()) {
    addr1 = RecordingBufferTmp1[i][0];
    addr2 = RecordingBufferTmp2[i][0];

    if (addr1 == addr2) { 
      insertInstruction(RecordingBufferTmp1[i]);
    } else {
      break;
    }

    i++;
  }
}

uint32_t MRET2::getStoredIndex(uint32_t addr) {
  for (int i = 0; i < 1000; i++) 
    if (stored[i].size() > 0 && stored[i][0][0] == addr) return i;
  return 1001;
}

uint32_t MRET2::getPhase(uint32_t addr) {
  if (phases.count(addr) == 0) phases[addr] = 1;
  return phases[addr];
}

void MRET2::finishPhase() {
  if (getPhase(RecordingEntry) == 1) {
    stored[stored_index] = RecordingBufferTmp1;

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
    RecordingBufferTmp1.clear();
    RecordingBufferTmp2.clear();
    phases[RecordingEntry] = 1;
    finishRegionFormation();
  }
}

void MRET2::onBranch(Machine& M) {
  if (Recording) { 
    for (uint32_t I = LastTarget; I <= M.getLastPC(); I += 4) {
      if (isBackwardLoop(I) || TheManager.isRegionEntry(I)) { 
        finishRegionFormation(); 
        break;
      }
      RecordingBufferTmp1.push_back({I, M.getInstAt(I).asI_});
    }

    if (TheManager.isNativeRegionEntry(M.getPC()))
      finishPhase();
  }

  if (TheManager.isNativeRegionEntry(M.getPC())) {
    auto Next = TheManager.jumpToRegion(M.getPC()); 
    M.setPC(Next);

    ++ExecFreq[M.getPC()];
    if (ExecFreq[M.getPC()] > HotnessThreshold/2 && isAllowedInstToStart(M.getPC(), M)) {
      startRegionFormation(M.getPC());
      RecordingBufferTmp1.clear();
      if (getPhase(M.getPC()) == 1)
        ExecFreq[M.getPC()] = 0;
    }
  } 

  if (M.getPC() < M.getLastPC()) {
    if (!Recording) { 
      ++ExecFreq[M.getPC()];
      if (!TheManager.isRegionEntry(M.getPC()) && ExecFreq[M.getPC()] > HotnessThreshold/2 
            && isAllowedInstToStart(M.getPC(), M)) { 
        startRegionFormation(M.getPC());
        RecordingBufferTmp1.clear();
        if (getPhase(M.getPC()) == 1)
          ExecFreq[M.getPC()] = 0;
      }
    } else {
      finishPhase();
    }
  }

  LastTarget = M.getPC();
}
