/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "phasar/Utils/Utilities.h"

#include "phasar/Utils/Logger.h"

#include "llvm/ADT/StringExtras.h"
#include "llvm/Demangle/Demangle.h"
#include "llvm/Demangle/ItaniumDemangle.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/Support/Allocator.h"

#include <algorithm>
#include <chrono>

using namespace std;
using namespace psr;

namespace psr {

std::string createTimeStamp() {
  auto Now = std::chrono::system_clock::now();
  auto NowTime = std::chrono::system_clock::to_time_t(Now);
  std::string TimeStr(std::ctime(&NowTime));
  std::replace(TimeStr.begin(), TimeStr.end(), ' ', '-');
  TimeStr.erase(std::remove(TimeStr.begin(), TimeStr.end(), '\n'),
                TimeStr.end());
  return TimeStr;
}

namespace {
// See llvm/Demangle/ItaniumDemangle.cpp
class DefaultAllocator {
  llvm::BumpPtrAllocator Alloc;

public:
  void reset() { Alloc.Reset(); }

  template <typename T, typename... ArgTys> T *makeNode(ArgTys &&...Args) {
    return new (Alloc.Allocate<T>()) T(std::forward<ArgTys>(Args)...);
  }

  void *allocateNodeArray(size_t Sz) {
    return Alloc.Allocate(sizeof(llvm::itanium_demangle::Node *) * Sz,
                          alignof(llvm::itanium_demangle::Node *));
  }
};
} // namespace

bool isConstructor(llvm::StringRef MangledName) {
  // See llvm/Demangle/ItaniumDemangle.cpp

  using namespace llvm::itanium_demangle;

  ManglingParser<DefaultAllocator> Parser{nullptr, nullptr};
  Parser.reset(MangledName.begin(), MangledName.end());
  const auto *N = Parser.parse();
  if (!N) {
    PHASAR_LOG_LEVEL(WARNING,
                     "Attempting to demangle a non-itanium ABI mangled name");
    return false;
  }

  // See llvm::ItaniumPartialDemangler::isCtorDtor()
  while (N) {
    switch (N->getKind()) {
    default:
      return false;
    case Node::KCtorDtorName: {
      bool Ret;
      static_cast<const CtorDtorName *>(N)->match( // NOLINT
          [&Ret](const Node * /*N*/, bool IsDtor, int /*Variant*/) {
            Ret = !IsDtor;
          });
      return Ret;
    }
    case Node::KAbiTagAttr:
      N = static_cast<const AbiTagAttr *>(N)->Base; // NOLINT
      break;
    case Node::KFunctionEncoding:
      N = static_cast<const FunctionEncoding *>(N)->getName(); // NOLINT
      break;
    case Node::KLocalName:
      N = static_cast<const LocalName *>(N)->Entity; // NOLINT
      break;
    case Node::KNameWithTemplateArgs:
      N = static_cast<const NameWithTemplateArgs *>(N)->Name; // NOLINT
      break;
    case Node::KNestedName:
      N = static_cast<const NestedName *>(N)->Name; // NOLINT
      break;
      // case Node::KModuleEntity:
      //   N = static_cast<const ModuleEntity *>(N)->Name; // NOLINT
      //   break;
    }
  }
  return false;
}

const llvm::Type *stripPointer(const llvm::Type *Pointer) {
  const auto *Next = llvm::dyn_cast<llvm::PointerType>(Pointer);
  while (Next) {
    Pointer = Next->getElementType();
    Next = llvm::dyn_cast<llvm::PointerType>(Pointer);
  }

  return Pointer;
}

bool isMangled(llvm::StringRef Name) {
  // See llvm/Demangle/Demangle.cpp
  if (Name.startswith("_Z") || Name.startswith("___Z")) {
    // Itanium ABI
    return true;
  }
  if (Name.startswith("_R")) {
    // Rust ABI
    return true;
  }
  if (Name.startswith("_D")) {
    // D ABI
    return true;
  }
  // Microsoft ABI is a bit more complicated...
  return Name != llvm::demangle(Name.str());
}

bool StringIDLess::operator()(const std::string &Lhs,
                              const std::string &Rhs) const {
  char *Endptr1;

  char *Endptr2;
  long LhsVal = strtol(Lhs.c_str(), &Endptr1, 10);
  long RhsVal = strtol(Rhs.c_str(), &Endptr2, 10);
  if (Lhs.c_str() == Endptr1 && Lhs.c_str() == Endptr2) {
    return Lhs < Rhs;
  }
  if (Lhs.c_str() == Endptr1 && Rhs.c_str() != Endptr2) {
    return false;
  }
  if (Lhs.c_str() != Endptr1 && Rhs.c_str() == Endptr2) {
    return true;
  }
  return LhsVal < RhsVal;
}

} // namespace psr
