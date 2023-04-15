#include "phasar/Utils/BitSetHash.h"

[[nodiscard]] llvm::hash_code
psr::detail::bvHashingHelper(llvm::ArrayRef<uintptr_t> Words) noexcept {
  size_t Idx = Words.size();
  while (Idx && Words[Idx - 1] == 0) {
    --Idx;
  }
  return llvm::hash_combine_range(Words.begin(),
                                  std::next(Words.begin(), ptrdiff_t(Idx)));
}
