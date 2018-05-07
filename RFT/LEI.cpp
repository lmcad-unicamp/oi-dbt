#include <RFT.hpp>

using namespace dbt;

void LEI::circularBufferInsert(uint32_t src, uint32_t tgt) {
  if (Buffer.size() > MAX_SIZE_BUFFER) {
    Buffer.clear();
    BufferHash.clear();
  }
  Buffer.push_back({src, tgt});
}

void LEI::formTrace(uint32_t start, int old, Machine& M) {
  startRegionFormation(start);
  unsigned long long prev = start;
  unsigned branch = old + 1;
  while (branch < Buffer.size()) {
    unsigned long long branch_src = Buffer[branch].src;
    unsigned long long branch_tgt = Buffer[branch].tgt;

    for (unsigned long long InstAddrs = prev; InstAddrs <= branch_src; InstAddrs += 4) {
        // Stop if next instruction begins a trace or is a syscall
        if (TheManager.isRegionEntry(InstAddrs) ||
          OIDecoder::decode(M.getInstAt(InstAddrs).asI_).Type == OIDecoder::OIInstType::Sqrts ||
          OIDecoder::decode(M.getInstAt(InstAddrs).asI_).Type == OIDecoder::OIInstType::Sqrtd) 
          break;
        insertInstruction(InstAddrs, M.getInstAt(InstAddrs).asI_);
    }

    // Stop if branch forms a cycle
    if (hasRecordedAddrs(branch_tgt)) 
      break;

    prev = branch_tgt;
    branch += 1;
  }
  finishRegionFormation();
}

bool LEI::isFollowedByExit(int Old) {
  uint32_t a = Buffer[Old].src;
  uint32_t b = Buffer[Old].tgt;
  return TheManager.inCodeCache(a) || !TheManager.inCodeCache(b);
}

void LEI::onBranch(Machine& M) {
  if (TheManager.isNativeRegionEntry(M.getPC())) {
    M.setPC(TheManager.jumpToRegion(M.getPC())); 
    return;
  }

  uint32_t src = M.getLastPC();
  uint32_t tgt = M.getPC();

  circularBufferInsert(src, tgt);
  if (BufferHash.count(tgt) != 0) {
    int old = BufferHash[tgt];
    BufferHash[tgt] = Buffer.size()-1;

    if (OIDecoder::isIndirectBranch(OIDecoder::decode(M.getInstAt(M.getLastPC()).asI_)))
      setBranchTarget(M.getLastPC(), M.getPC());

    // if tgt ≤ src or old follows exit from code cache
    bool is_a_cache_exit = isFollowedByExit(old);
    if (tgt <= src || is_a_cache_exit) {
      // increment counter c associated with tgt
      ++ExecFreq[tgt];

      // if c = Tcyc
      if (ExecFreq[tgt] > HotnessThreshold) {
        formTrace(tgt, old, M);

        // remove all elements of Buf after old
        for (unsigned I = old; I < Buffer.size(); I++) 
          BufferHash.erase(Buffer[I].tgt);
        Buffer.erase(Buffer.begin()+old, Buffer.end());

        // recycle counter associated with tgt
        ExecFreq[tgt] = 0;

        while (TheManager.getNumOfOIRegions() != 0) {}

        // jump newT
        if (TheManager.isNativeRegionEntry(tgt)) 
          M.setPC(TheManager.jumpToRegion(tgt)); 
      }
    }
  } else {
    BufferHash[tgt] = Buffer.size()-1;
  }
}
