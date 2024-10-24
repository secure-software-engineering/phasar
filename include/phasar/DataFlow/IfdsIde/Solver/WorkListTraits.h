#pragma once

#include "phasar/DataFlow/IfdsIde/Solver/Compressor.h"

#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/Compiler.h"

#include <cassert>
#include <deque>
#include <functional>
#include <vector>
namespace psr {

template <typename T> class VectorWorkList {
public:
  VectorWorkList() noexcept = default;
  template <typename NodeCompressor, typename ICFGTy>
  VectorWorkList(const NodeCompressor &NC, const ICFGTy &ICF) noexcept {}

  void reserve(size_t Capacity) { WL.reserve(Capacity); }

  [[nodiscard]] bool empty() const noexcept { return WL.empty(); }

  template <typename... ArgTys> void emplace(ArgTys &&...Args) {
    WL.emplace_back(std::forward<ArgTys>(Args)...);
  }

  template <typename HandlerFun>
  void processEntriesUntilEmpty(HandlerFun Handler) {
    while (!WL.empty()) {
      auto Item = std::move(WL.back());
      WL.pop_back();
      std::invoke(Handler, std::move(Item));
    }
  }

  void clear() noexcept {
    WL.clear();
    WL.shrink_to_fit();
  }

  [[nodiscard]] LLVM_DUMP_METHOD size_t size() const noexcept {
    return WL.size();
  }

private:
  std::vector<T> WL;
};

template <typename T, size_t N> class SmallVectorWorkList {
  SmallVectorWorkList() noexcept = default;
  template <typename NodeCompressor, typename ICFGTy>
  SmallVectorWorkList(const NodeCompressor &NC, const ICFGTy &ICF) noexcept {}

  void reserve(size_t Capacity) { WL.reserve(Capacity); }

  [[nodiscard]] bool empty() const noexcept { return WL.empty(); }

  template <typename... ArgTys> void emplace(ArgTys &&...Args) {
    WL.emplace_back(std::forward<ArgTys>(Args)...);
  }

  template <typename HandlerFun>
  void processEntriesUntilEmpty(HandlerFun Handler) {
    while (!WL.empty()) {
      auto Item = WL.pop_back_val();
      std::invoke(Handler, std::move(Item));
    }
  }

  void clear() noexcept {
    /// llvm::SmallVector does not have a shrink_to_fit() function, so use this
    /// workaround:
    llvm::SmallVector<T, N> Empty;
    swap(Empty, WL);
  }

  [[nodiscard]] size_t size() const noexcept { return WL.size(); }

private:
  llvm::SmallVector<T, N> WL;
};

template <typename T> class DequeWorkList {
public:
  DequeWorkList() noexcept = default;

  template <typename NodeCompressor, typename ICFGTy>
  DequeWorkList(const NodeCompressor &NC, const ICFGTy &ICF) noexcept {}

  void reserve(size_t Capacity) { WL.reserve(Capacity); }

  [[nodiscard]] bool empty() const noexcept { return WL.empty(); }

  template <typename... ArgTys> void emplace(ArgTys &&...Args) {
    WL.emplace_back(std::forward<ArgTys>(Args)...);
  }

  template <typename HandlerFun>
  void processEntriesUntilEmpty(HandlerFun Handler) {
    while (!WL.empty()) {
      auto Item = std::move(WL.front());
      WL.pop_front();
      std::invoke(Handler, std::move(Item));
    }
  }

  void clear() noexcept {
    WL.clear();
    WL.shrink_to_fit();
  }

  [[nodiscard]] size_t size() const noexcept { return WL.size(); }

private:
  std::deque<T> WL;
};

} // namespace psr
