
#include "phasar/Utils/MaybeUniquePtr.h"

#include <memory>

struct IncompleteType;

using MUP = psr::MaybeUniquePtr<IncompleteType, true>;
MUP maybeUniquePtrSupportsIncompleteTypes(
    std::unique_ptr<IncompleteType> &&UP) {
  return std::move(UP);
}

int main() {}
