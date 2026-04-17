#ifndef PHASAR_DATAFLOW_IFDSIDE_SOLVER_COMPRESSOR_H
#define PHASAR_DATAFLOW_IFDSIDE_SOLVER_COMPRESSOR_H

#include "phasar/Utils/ByRef.h"
#include "phasar/Utils/Compressor.h"

#include <cstdint>
#include <type_traits>

namespace psr {

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
