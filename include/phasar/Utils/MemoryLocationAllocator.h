/******************************************************************************
 * Copyright (c) 2025 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_UTILS_MEMORYLOCATIONALLOCATOR_H
#define PHASAR_UTILS_MEMORYLOCATIONALLOCATOR_H

#include "llvm/Support/Compiler.h"
#include "llvm/Support/TrailingObjects.h"

#include <utility>

namespace psr {
/// \brief A simple custom allocator that is used to allocate, e.g.,
/// AbstractMemoryLocation for the IDEExtendedTaintAnalysis.
///
/// It can be used to allocate trivially destructible objects of variable size,
/// e.g., using llvm::TrailingObjects, that should live as long as the
/// allocator.
class MemoryLocationAllocator {
public:
  constexpr MemoryLocationAllocator() noexcept = default;

  explicit MemoryLocationAllocator(size_t InitialCapacity,
                                   size_t DynamicBlockSize = size_t(1024) * 6 -
                                                             8);

  constexpr MemoryLocationAllocator(MemoryLocationAllocator &&Other) noexcept
      : Root(Other.Root), Pos(Other.Pos), End(Other.End) {
    Other.Root = nullptr;
    Other.Pos = nullptr;
    Other.End = nullptr;
  }
  MemoryLocationAllocator &operator=(MemoryLocationAllocator &&Other) noexcept {
    MemoryLocationAllocator(std::move(Other)).swap(*this);
    return *this;
  }

  ~MemoryLocationAllocator();

  MemoryLocationAllocator(const MemoryLocationAllocator &) = delete;
  MemoryLocationAllocator &operator=(const MemoryLocationAllocator &) = delete;

  void swap(MemoryLocationAllocator &Other) noexcept {
    std::swap(Root, Other.Root);
    std::swap(Pos, Other.Pos);
    std::swap(End, Other.End);
  }

  /// \brief Allocates a chunk of memory of at least NumBytes bytes, aligned to
  /// alignof(void *).
  ///
  /// Fails with llvm::report_bad_alloc_error(), if memory could not be
  /// allocated.
  [[nodiscard]] LLVM_ATTRIBUTE_RETURNS_NONNULL void *allocate(size_t NumBytes);

private:
  struct Block final : public llvm::TrailingObjects<Block, void *> {

    Block *Next = nullptr;

    static Block *create(Block *Next, size_t NumPointerEntries);
    static void destroy(Block *Blck, size_t NumPointerEntries);

  private:
    constexpr Block(Block *Next) noexcept : Next(Next) {}
  };

  Block *Root = nullptr;
  void **Pos = nullptr;
  void **End = nullptr;
  size_t InitialCapacity{};
  size_t DynamicBlockSize{};
};
} // namespace psr

#endif // PHASAR_UTILS_MEMORYLOCATIONALLOCATOR_H
