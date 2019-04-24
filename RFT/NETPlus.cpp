#include <RFT.hpp>
#include <OIPrinter.hpp>

#include <queue>
#include <set>

#include <memory>

using namespace dbt;

#define InstPair std::array<uint32_t, 2>

bool isControlFlowInst(uint32_t Opcode, bool IsCallExtended) {
  OIDecoder::OIInst Inst = OIDecoder::decode(Opcode);
  return OIDecoder::isControlFlowInst(Inst) || (IsCallExtended && Inst.Type == Jumpr);
}

std::array<uint32_t, 2> getPossibleNextAddrs(InstPair Branch) {
  OIDecoder::OIInst Inst = OIDecoder::decode(Branch[1]);
  return OIDecoder::getPossibleTargets(Branch[0], Inst);
}

void NETPlus::addNewPath(OIInstList NewPath) {
  std::reverse(NewPath.begin(), NewPath.end());
  for (auto NewInst : NewPath) 
    insertInstruction(NewInst[0], NewInst[1]);
}

void NETPlus::expand(unsigned Deepness, Machine& M) {
  if (OIRegion.size() == 0) return;

  std::queue<InstPair> S;
  spp::sparse_hash_map<uint32_t, unsigned> Distance;
  spp::sparse_hash_map<uint32_t, InstPair> Next, Parent, CallStack;
  std::set<uint32_t> LoopEntries;

  // Init BFS frontier
  bool First = true;
  for (auto I : OIRegion) {
    uint32_t Addrs = I[0];

    if (First) {
      LoopEntries.insert(Addrs);
      First = false;
    } else {
      if (isControlFlowInst(I[1], IsCallExtended)) {
        S.push(I);
        Distance[Addrs] = 0;
        Parent[Addrs] = {0, 0};
      }
    }
  }

  // Add last instruction as possible exit to expand
  if (!isControlFlowInst((OIRegion.back()[1]), IsCallExtended)) {
    S.push(OIRegion.back());
    Distance[OIRegion.back()[0]] = 0;
    Parent[OIRegion.back()[0]] = {0, 0};
  }

  while (!S.empty()) {
    InstPair Current = S.front();
    S.pop();

    if (Distance[Current[0]] < Deepness) {
      std::array<uint32_t, 2> NextAddrs = {0, 0};
      if (OIDecoder::decode(Current[1]).Type == Jumpr) {
        uint32_t Begin = Current[0];
        uint32_t Prev = Next[Begin][0];
        while (true) {
          if (OIDecoder::decode(Parent[Prev][1]).Type == Call || OIDecoder::decode(Parent[Prev][1]).Type == Callr) {
            NextAddrs = {Parent[Prev][0]+4, 0};
            break;
          }
          Begin = Parent[Prev][0];
          Prev  = Next[Begin][0];
          if (Prev == 0) {
            break;
          }
        }
      } else {
        NextAddrs = getPossibleNextAddrs(Current);
      }

      for (auto Target : NextAddrs) {
        if (Target == 0 || Parent.count(Target) != 0 || TheManager.isRegionEntry(Target)) continue;

        Parent[Target] = Current;
        // Iterate over all instructions between the target and the next branch
        InstPair it = {Target, M.getInstAt(Target).asI_};

        while (it[0] < M.getCodeEndAddrs()) {
          bool IsCycle = (hasRecordedAddrs(it[0]) && IsExtendedRelaxed) || (OIRegion[0][0] == it[0] && !IsExtendedRelaxed);

          // Either it found a cycle and add a new path
          if (IsCycle && Distance[Current[0]] > 0 || (!IsCallExtended && OIDecoder::decode(it[1]).Type == Jumpr)) {
            OIInstList NewPath;
            uint32_t Begin = it[0];
            uint32_t Prev = Target;
            while (true) {
              auto i = Begin;
              while (true) {
                NewPath.push_back({i, M.getInstAt(i).asI_});
                if (i == Prev) break;
                i -= 4;
              }
              Begin = Parent[Prev][0];
              Prev  = Next[Begin][0];
              if (Prev == 0) break;
            }
            addNewPath(NewPath);
            break;
          }
          
          // Or it found another branch
          if (isControlFlowInst(it[1], IsCallExtended) && Distance.count(it[0]) == 0) {
            S.push(it);
            Distance[it[0]] = Distance[Current[0]] + 1;
            Next[it[0]] = { Target, M.getInstAt(Target).asI_ };
            break;
          }
          it = {it[0]+4, M.getInstAt(it[0]+4).asI_};
        }
      }
    }
  }
}

void NETPlus::expandAndFinish(Machine& M) {
  expand(10, M); 
  finishRegionFormation(); 
}

static unsigned int regionFrequency= 0;

void NETPlus::onBranch(Machine& M) {
  
  if (Recording) {
    for (uint32_t I = LastTarget; I <= M.getLastPC(); I += 4) {
      if ((IsExtendedRelaxed ? hasRecordedAddrs(I) : isBackwardLoop(I)) || TheManager.isRegionEntry(I)) { 
        expandAndFinish(M);
        break;
      }
      OIRegion.push_back({I, M.getInstAt(I).asI_});
    }

    if (TheManager.isNativeRegionEntry(M.getPC())) 
      expandAndFinish(M);

  } else if (M.getPC() < M.getLastPC() && !TheManager.isRegionEntry(M.getPC())) {
    ++ExecFreq[M.getPC()];
    if (ExecFreq[M.getPC()] > HotnessThreshold) 
      startRegionFormation(M.getPC());
  }

  if (TheManager.isNativeRegionEntry(M.getPC())) {
    auto Next = TheManager.jumpToRegion(M.getPC()); 
    M.setPC(Next);

    ++ExecFreq[Next];
    if (ExecFreq[M.getPC()] > HotnessThreshold)
      startRegionFormation(Next);

    TheManager.setRegionRecorging(false);
  }  

  LastTarget = M.getPC();
}
