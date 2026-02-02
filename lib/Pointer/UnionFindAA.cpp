
#include "phasar/Pointer/UnionFindAA.h"

#include "phasar/Utils/StrongTypeDef.h" // for to_underlying

using namespace psr;

void psr::UnionFindAAResultBase::dump(llvm::raw_ostream &OS,
                                      uint32_t Indent) const {
  OS.indent(Indent) << "UnionFindAAResult {\n";

  for (const auto &[RepId, Aliases] : BackwardView.enumerate()) {
    OS.indent(Indent + 2) << "#" << psr::to_underlying(RepId) << ": <";
    bool First = true;
    Aliases.foreach ([&](auto AliasId) {
      if (First) {
        First = false;
      } else {
        OS << ", ";
      }

      OS << psr::to_underlying(AliasId);
    });
    OS << ">\n";
  }

  OS.indent(Indent) << "}\n";
}
