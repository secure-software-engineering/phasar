#ifndef PHASAR_DATAFLOW_IFDSIDE_SOLVER_COMPRESSOR_H
#define PHASAR_DATAFLOW_IFDSIDE_SOLVER_COMPRESSOR_H

#include "phasar/Utils/ByRef.h"
#include "phasar/Utils/Compressor.h"

#include <cstdint>
#include <type_traits>

namespace psr {

struct NoneCompressor final {
  constexpr NoneCompressor() noexcept = default;

  template <typename T>
    requires(!std::is_same_v<NoneCompressor, T>)
  constexpr NoneCompressor(const T & /*unused*/) noexcept {}

  template <typename T>
  [[nodiscard]] decltype(auto) getOrInsert(T &&Val) const noexcept {
    return std::forward<T>(Val);
  }
  template <typename T>
  [[nodiscard]] decltype(auto) operator[](T &&Val) const noexcept {
    return std::forward<T>(Val);
  }
  void reserve(size_t /*unused*/) const noexcept {}

  [[nodiscard]] size_t size() const noexcept { return 0; }
  [[nodiscard]] size_t capacity() const noexcept { return 0; }
};

class LLVMProjectIRDB;

/// Once we have fast instruction IDs (as we already have in IntelliSecPhasar),
/// we might want to create a specialization for T/const llvm::Value * that uses
/// the IDs from the IRDB
template <typename T> struct NodeCompressorTraits {
  using type = Compressor<T>;

  static type
  create(const LLVMProjectIRDB * /*IRDB*/) noexcept(noexcept(type())) {
    return type();
  }
};

template <typename T> struct ValCompressorTraits {
  using type = Compressor<T>;
  using id_type = uint32_t;
};

template <typename T>
  requires CanEfficientlyPassByValue<T>
struct ValCompressorTraits<T> {
  using type = NoneCompressor;
  using id_type = T;
};

} // namespace psr

#endif // PHASAR_DATAFLOW_IFDSIDE_SOLVER_COMPRESSOR_H
