#include <manager.hpp>
#include <OIDecoder.hpp>

using namespace dbt;

bool contains(uint32_t Addr, OIInstList OIInts) {
  for (auto Inst : OIInts) 
    if (Inst[0] == Addr)
      return true;
  return false;
}

void Manager::mergeOIRegions() { 
  std::cerr << "Merging\n";
  bool Changed = true;
  while (Changed) {
    Changed = false;
    for (auto& OIRegion : CompiledOIRegions) {
      for (auto OIInsts : OIRegion.second) {
        OIInst Inst = OIDecoder::decode(OIInsts[1]);
        if (OIDecoder::isControlFlowInst(Inst)) {
          auto Target = OIDecoder::getPossibleTargets(OIInsts[0], Inst);
          if (!contains(Target[0], OIRegion.second) && CompiledOIRegions.count(Target[0]) != 0) {          
            int i = 0;
            for (auto OIInsts : CompiledOIRegions[Target[0]]) 
              if (!contains(OIInsts[0], OIRegion.second))
                i++;
            if (i != 0) {
              for (auto OIInsts : CompiledOIRegions[Target[0]]) 
                if (!contains(OIInsts[0], OIRegion.second))
                  OIRegion.second.push_back({OIInsts[0], OIInsts[1]});
              std::cerr << OIInsts[0] << " jumping to other region: " << Target[0] << "! Inlining it should add " << i << " instructions \n";
              Changed = true;
            }
          }
          if (Target[1] != 0 && !contains(Target[1], OIRegion.second) && CompiledOIRegions.count(Target[1]) != 0) {
            int i = 0;
            for (auto OIInsts : CompiledOIRegions[Target[0]]) 
              if (!contains(OIInsts[0], OIRegion.second))
                i++;
            if (i != 0) {
              for (auto OIInsts : CompiledOIRegions[Target[0]]) 
                if (!contains(OIInsts[0], OIRegion.second))
                  OIRegion.second.push_back({OIInsts[0], OIInsts[1]});
              std::cerr << OIInsts[0] <<" jumping to other region: " << Target[1] << "! Inlining it should add " << i << " instructions \n";
              Changed = true;
            }
          }
        }
      }
    }
  }
}


