#ifndef PHASAR_UTILS_JOINLATTICEVERIFIER_H
#define PHASAR_UTILS_JOINLATTICEVERIFIER_H

#include "phasar/Utils/JoinLattice.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/raw_ostream.h"

#include <functional>
#include <unordered_set>
#include <utility>

namespace psr {
namespace detail {
template <typename L>
[[nodiscard]] inline bool respectsTopBot(const L &Val, llvm::raw_ostream &Err) {
  bool Ret = true;
  const L &Top = JoinLatticeTraits<L>::top();
  const L &Bot = JoinLatticeTraits<L>::bottom();

  {
    const auto &JoinWithTop = JoinLatticeTraits<L>::join(Val, Top);
    const auto &JoinWithTopR = JoinLatticeTraits<L>::join(Top, Val);

    if (JoinWithTop != Val) {
      Err << "Top is not absorbed: join(" << Val << ", " << Top
          << ") = " << JoinWithTop << "; expected " << Val << '\n';
      Ret = false;
    }
    if (JoinWithTopR != Val) {
      Err << "Top is not absorbed: join(" << Top << ", " << Val
          << ") = " << JoinWithTopR << "; expected " << Val << '\n';
      Ret = false;
    }
  }
  {
    const auto &JoinWithBot = JoinLatticeTraits<L>::join(Val, Bot);
    const auto &JoinWithBotR = JoinLatticeTraits<L>::join(Bot, Val);

    if (JoinWithBot != Bot) {
      Err << "Bot is not the largest element in the lattice: join(" << Val
          << ", " << Bot << ") = " << JoinWithBot << "; expected " << Bot
          << '\n';
      Ret = false;
    }
    if (JoinWithBotR != Bot) {
      Err << "Bot is not the largest element in the lattice: join(" << Bot
          << ", " << Val << ") = " << JoinWithBot << "; expected " << Bot
          << '\n';
      Ret = false;
    }
  }

  return Ret;
}

template <typename L>
[[nodiscard]] inline bool idempotentJoin(const L &Val, llvm::raw_ostream &Err) {
  const auto &JoinWithSelf = JoinLatticeTraits<L>::join(Val, Val);
  if (JoinWithSelf != Val) {
    Err << "The join operation is not idempotent: join(" << Val << ", " << Val
        << ") = " << JoinWithSelf << "; expected " << Val << '\n';
    return false;
  }
  return true;
}

template <typename L>
[[nodiscard]] inline bool isPartiallyOrderedWithJoin(const L &Val,
                                                     const L &Other,
                                                     llvm::raw_ostream &Err) {
  const auto &Join = JoinLatticeTraits<L>::join(Val, Other);
  const auto &JoinR = JoinLatticeTraits<L>::join(Other, Val);
  bool Ret = true;

  if (Join != JoinR) {
    Err << "The join operation is not commutative: expect join(" << Val << ", "
        << Other << ") == join(" << Other << ", " << Val
        << "), but got: " << Join << " != " << JoinR << '\n';
    Ret = false;
  }

  const auto &JoinWithJoined = JoinLatticeTraits<L>::join(Val, Join);
  const auto &JoinWithJoinedR = JoinLatticeTraits<L>::join(Join, Val);

  if (JoinWithJoined != Join) {
    Err << "The join operation does not compute an upper bound: expect " << Val
        << " <= join(" << Val << ", " << Other << ") [which is: " << Join
        << "], but join(" << Val << ", " << Join << ") = " << JoinWithJoined
        << "; expect " << Join << '\n';
    Ret = false;
  }
  if (JoinWithJoinedR != Join) {
    Err << "The join operation does not compute an upper bound: expect " << Val
        << " <= join(" << Val << ", " << Other << ") [which is: " << Join
        << "], but join(" << Val << ", " << Join << ") = " << JoinWithJoinedR
        << "; expect " << Join << '\n';
    Ret = false;
  }

  return Ret;
}

} // namespace detail

template <typename L, typename GeneratorT, typename HasherT = std::hash<L>,
          typename EqualT = std::equal_to<L>>
[[nodiscard]] inline bool
validateBoundedJoinLattice(const GeneratorT &Generator,
                           llvm::raw_ostream &Err = llvm::errs(),
                           HasherT Hasher = {}, EqualT Equal = {}) {

  llvm::SmallVector<L> WL(llvm::adl_begin(Generator), llvm::adl_end(Generator));
  llvm::SmallVector<L> All = WL;
  llvm::SmallVector<L> Next;

  std::unordered_set<L, HasherT, EqualT> Seen(
      WL.begin(), WL.end(), WL.size(), std::move(Hasher), std::move(Equal));

  bool Ret = true;

  while (true) {

    for (const L &Val : WL) {

      Ret &= detail::idempotentJoin(Val, Err);
      Ret &= detail::respectsTopBot(Val, Err);

      for (const L &Other : All) {
        Ret &= detail::isPartiallyOrderedWithJoin(Val, Other, Err);

        auto &&Join = JoinLatticeTraits<L>::join(Val, Other);

        if (Seen.insert(Join).second) {
          Next.push_back(std::forward<decltype(Join)>(Join));
        }
      }
    }

    if (!Ret || Next.empty()) {
      return Ret;
    }

    All.append(Next);
    WL = std::exchange(Next, {});
  }
}
} // namespace psr

#endif // PHASAR_UTILS_JOINLATTICEVERIFIER_H
