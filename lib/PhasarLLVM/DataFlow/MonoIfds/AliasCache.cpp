#include "phasar/PhasarLLVM/DataFlow/MonoIfds/AliasCache.h"

#include "phasar/Utils/Logger.h"
#include "phasar/Utils/Printer.h"

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
    PHASAR_LOG_LEVEL_CAT(DEBUG, LogCategory, "For " << DToString(Fact));
    AI.forallAliasesOf(Fact, At, [this, &Vec = It->second](const auto *Alias) {
      PHASAR_LOG_LEVEL_CAT(DEBUG, LogCategory, "  Alias " << DToString(Alias));
      const auto *AliasBase = Alias->stripPointerCastsAndAliases();
      if (const auto *Glob = llvm::dyn_cast<llvm::GlobalVariable>(AliasBase);
          Glob && !PermittedGlobals->contains(Glob)) {
        return;
      }
      if (!SkipSeedsCallBack || !SkipSeedsCallBack(Alias)) {
        PHASAR_LOG_LEVEL_CAT(DEBUG, LogCategory, "  --> add");
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
