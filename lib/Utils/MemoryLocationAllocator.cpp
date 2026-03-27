#include "phasar/Utils/MemoryLocationAllocator.h"

#include "phasar/Utils/Logger.h"

#include "llvm/Support/Compiler.h"
#include "llvm/Support/ErrorHandling.h"

using namespace psr;

MemoryLocationAllocator::Block *
MemoryLocationAllocator::Block::create(Block *Next, size_t NumPointerEntries) {
  // Allocate one more pointer to store the next-block ptr

  if (LLVM_UNLIKELY(NumPointerEntries >
                    std::numeric_limits<size_t>::max() / sizeof(size_t) - 1)) {

    PHASAR_LOG_LEVEL(CRITICAL, "Cannot allocate " << NumPointerEntries
                                                  << " pointer entries");

    llvm::report_bad_alloc_error(
        "Cannot allocate memory for abstract memory locations");
  }

  static_assert(sizeof(Block) == sizeof(void *));
  static_assert(alignof(Block) == alignof(void *));
  auto *RetBytes = new void *[1 + NumPointerEntries];

  auto *Ret = new (RetBytes) Block(Next);

  __asan_poison_memory_region(Ret->getTrailingObjects<void *>(),
                              NumPointerEntries * sizeof(void *));

  return Ret;
}

void MemoryLocationAllocator::Block::destroy(
    MemoryLocationAllocator::Block *Blck,
    [[maybe_unused]] size_t NumPointerEntries) {
  __asan_unpoison_memory_region(Blck->getTrailingObjects<void *>(),
                                NumPointerEntries * sizeof(void *));
  delete[] reinterpret_cast<void **>(Blck);
}

static constexpr size_t translateBytesToPointers(size_t Bytes) noexcept {
  return (Bytes + sizeof(void *)) / sizeof(void *);
}

MemoryLocationAllocator::MemoryLocationAllocator(size_t InitialCapacity,
                                                 size_t DynamicBlockSize)
    : InitialCapacity(translateBytesToPointers(InitialCapacity)),
      DynamicBlockSize(translateBytesToPointers(DynamicBlockSize)) {
  assert(DynamicBlockSize >= sizeof(void *));
  if (this->InitialCapacity <= this->DynamicBlockSize) {
    return;
  }

  Root = Block::create(nullptr, this->InitialCapacity);
  Pos = Root->getTrailingObjects<void *>();
  End = Pos + this->InitialCapacity;
}

MemoryLocationAllocator::~MemoryLocationAllocator() {
  auto *Rt = Root;
  auto *Blck = Rt;
  while (Blck) {
    auto *Nxt = Blck->Next;
    Block::destroy(Blck, Blck == Rt ? InitialCapacity : DynamicBlockSize);
    Blck = Nxt;
  }
  Root = nullptr;
  Pos = nullptr;
  End = nullptr;
}

[[nodiscard]] LLVM_ATTRIBUTE_RETURNS_NONNULL void *
MemoryLocationAllocator::allocate(size_t NumBytes) {
  auto NumPointersRequired = translateBytesToPointers(NumBytes);
  if (LLVM_UNLIKELY(NumPointersRequired == 0)) {
    // Prevent aliasing issues by refusing to allocate zero bytes
    NumPointersRequired = 1;
  }

  auto *Rt = Root;
  auto *Curr = Pos;

  if (LLVM_UNLIKELY(End - Curr < ptrdiff_t(NumPointersRequired))) {
    Root = Rt = Block::create(Rt, DynamicBlockSize);
    Pos = Curr = Rt->getTrailingObjects<void *>();
    End = Curr + DynamicBlockSize;
  }

  auto *Ret = Curr;
  Pos += NumPointersRequired;

  __asan_unpoison_memory_region(Ret, NumPointersRequired * sizeof(void *));

  return Ret;
}
