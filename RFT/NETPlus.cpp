#include <RFT.hpp>
#include <OIPrinter.hpp>

#include <queue>
#include <set>

#include <memory>

using namespace dbt;

#define InstPair std::array<uint32_t, 2>

//#define LIMITED

#ifdef LIMITED
unsigned TotalInst = 0;
#endif

bool isControlFlowInst(uint32_t Opcode) {
  OIDecoder::OIInst Inst = OIDecoder::decode(Opcode);
  return OIDecoder::isControlFlowInst(Inst);
}

std::array<uint32_t, 2> getPossibleNextAddrs(InstPair Branch) {
  OIDecoder::OIInst Inst = OIDecoder::decode(Branch[1]);
  return OIDecoder::getPossibleTargets(Branch[0], Inst);
}

void NETPlus::addNewPath(OIInstList NewPath) {
  std::reverse(NewPath.begin(), NewPath.end());
  for (auto NewInst : NewPath) {
#ifdef LIMITED
    if (TotalInst < RegionLimitSize) {
      insertInstruction(NewInst[0], NewInst[1]);
      TotalInst++;
    } else if (TotalInst == RegionLimitSize) {
      for (auto I : OIRegion) {
        std::cerr << std::hex << I[0] << ":\t" << dbt::OIPrinter::getString(OIDecoder::decode(I[1])) << "\n";
      }
      std::cerr <<"BLAH: " << OIRegion[0][0] << "\n";
      TotalInst++;
      finishRegionFormation(); 
    }
#else
    insertInstruction(NewInst[0], NewInst[1]);
#endif
  }
}

void NETPlus::expand(unsigned Deepness, Machine& M) {
  if (OIRegion.size() == 0) return;

  std::queue<InstPair> S;
  spp::sparse_hash_map<uint32_t, unsigned> Distance;
  spp::sparse_hash_map<uint32_t, InstPair> Next, Parent;
  std::set<uint32_t> LoopEntries;

  // Init BFS frontier
  bool First = true;
  for (auto I : OIRegion) {
    uint32_t Addrs = I[0];

    if (First) {
      LoopEntries.insert(Addrs);
      First = false;
    } else {
      if (isControlFlowInst(I[1])) {
        S.push(I);
        Distance[Addrs] = 0;
        Parent[Addrs] = {0, 0};
      }
    }
  }

  while (!S.empty()) {
    InstPair Current = S.front();
    S.pop();

    if (Distance[Current[0]] < Deepness) {

      for (auto Target : getPossibleNextAddrs(Current)) {
        if (Target == 0 || Parent.count(Target) != 0) continue;

        Parent[Target] = Current;
        // Iterate over all instructions between the target and the next branch
        InstPair it = {Target, M.getInstAt(Target).asI_};

        while (it[0] < M.getCodeEndAddrs()) {
          bool IsCycle = ((hasRecordedAddrs(it[0]) && IsExtendedRelaxed) 
              || (OIRegion[0][0] == it[0] && !IsExtendedRelaxed));

          if (IsCycle && Distance[Current[0]] > 0) {
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
              if (Prev == 0) {
                break;
              }
            }

            addNewPath(NewPath);
            break;
          }

          if (TheManager.isRegionEntry(it[0])) 
            break;

          if (isControlFlowInst(it[1]) && Distance.count(it[0]) == 0) {
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
      if (OIRegion.size() > RegionMaxSize 
          || (IsExtendedRelaxed && hasRecordedAddrs(I))
          || (!IsExtendedRelaxed && (M.getPC() < M.getLastPC())) || TheManager.isRegionEntry(I)) { 
        expandAndFinish(M);
        break;
      }

#ifdef LIMITED      
      if (TotalInst < RegionLimitSize) {
        OIRegion.push_back({I, M.getInstAt(I).asI_});
        TotalInst++;
      } else if (TotalInst == RegionLimitSize) {
        for (auto I : OIRegion) {
          std::cerr << std::hex << I[0] << ":\t" << dbt::OIPrinter::getString(OIDecoder::decode(I[1])) << "\n";
        }
        std::cerr <<"BLAH: " << I << "\n";
        TotalInst++;
        finishRegionFormation(); 
      }
#else
      OIRegion.push_back({I, M.getInstAt(I).asI_});
#endif
    }
  } else if (abs(M.getPC() - M.getLastPC()) > 4 && !TheManager.isRegionEntry(M.getPC())) {
    ++ExecFreq[M.getPC()];
    if (ExecFreq[M.getPC()] > HotnessThreshold) 
      startRegionFormation(M.getPC());
  }

  if (TheManager.isNativeRegionEntry(M.getPC())) {
    if (Recording) 
      expandAndFinish(M);

    auto Next = TheManager.jumpToRegion(M.getPC()); 
    M.setPC(Next);

    ++ExecFreq[Next];
    if (ExecFreq[M.getPC()] > HotnessThreshold)
      startRegionFormation(Next);

    TheManager.setRegionRecorging(false);
  }  

  LastTarget = M.getPC();
}
