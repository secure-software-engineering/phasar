#include "phasar/PhasarLLVM/DataFlow/MonoIfds/AliasCache.h"

#include "llvm/IR/Instruction.h"

using namespace psr;

llvm::ArrayRef<const llvm::Value *>
monoifds::AliasCache::getAliasSet(const llvm::Value *Fact,
                                  const llvm::Instruction *At) {
  const auto *AtFun = At->getFunction();
  auto [It, Inserted] = Cache.try_emplace(std::make_pair(AtFun, Fact));
  static size_t Misses = 0;
  static size_t Accesses = 0;
  Accesses++;
  if (Inserted) {
    Misses++;
    AI.forallAliasesOf(Fact, At, [this, &Vec = It->second](const auto *Alias) {
      const auto *AliasBase = Alias->stripPointerCastsAndAliases();
      if (const auto *Glob = llvm::dyn_cast<llvm::GlobalVariable>(AliasBase);
          Glob && !PermittedGlobals->contains(Glob)) {
        return;
      }
      if (!SkipSeedsCallBack || !SkipSeedsCallBack(Alias)) {
        Vec.push_back(Alias);
      }
    });
  }

  static psr::scope_exit PrintStats = [] {
    llvm::errs() << "AliasCache: Accesses: " << Accesses
                 << "\n> Misses: " << Misses << '\n';
  };

  return It->second;
}
