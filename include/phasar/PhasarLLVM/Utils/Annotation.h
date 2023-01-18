#ifndef PHASAR_PHASARLLVM_UTILS_ANNOTATION_H
#define PHASAR_PHASARLLVM_UTILS_ANNOTATION_H

#include "llvm/ADT/StringRef.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Value.h"

#include <string>

namespace psr {

//===----------------------------------------------------------------------===//
// Helper classes that allow for an easier retrieval of annotation information.
//===----------------------------------------------------------------------===//

class VarAnnotation {
public:
  VarAnnotation(const llvm::CallBase *AnnotationCall) noexcept;

  [[nodiscard]] const llvm::Value *getValue() const;
  [[nodiscard]] llvm::StringRef getAnnotationString() const;
  [[nodiscard]] llvm::StringRef getFile() const;
  [[nodiscard]] uint64_t getLine() const;
  /// Removes the bitcast and returns the original value that has been annotated
  /// or returns the respective function arguments if the values originates from
  /// a function argument.
  [[nodiscard]] static const llvm::Value *
  getOriginalValueOrOriginalArg(const llvm::Value *AnnotatedValue);

private:
  const llvm::CallBase *AnnotationCall;

  [[nodiscard]] llvm::StringRef retrieveString(unsigned Idx) const;
};

class GlobalAnnotation {
public:
  GlobalAnnotation(const llvm::ConstantStruct *AnnotationStruct) noexcept;

  [[nodiscard]] const llvm::Function *getFunction() const;
  [[nodiscard]] llvm::StringRef getAnnotationString() const;
  [[nodiscard]] llvm::StringRef getFile() const;
  [[nodiscard]] uint64_t getLine() const;

private:
  const llvm::ConstantStruct *AnnotationStruct;

  [[nodiscard]] llvm::StringRef retrieveString(unsigned Idx) const;
};

} // namespace psr

#endif
