#include <FormatConverter/NFACreator.h>
#include <Parser/TokenHelper.h>
#include <algorithm>
#include <array>
#include <functional>
#include <stdexcept>
#include <unordered_set>
#include <utility>
#include <vector>

namespace CCPP {
using namespace std;
using namespace DFA;
/// \brief A helper-class, which is able to create a nondeterministic state
/// machine from a CrySLParser::OrderContext *
class NFACreator {
  CrySLParser::OrderContext *order;
  CrySLParser::EventsContext *events;
  StateMachine &NFA;
  OrderConverter &orc;
  /// \brief Executes the callBack for each permutation of the vector vec
  template <typename T>
  void forEachPermutation(vector<T> &&vec,
                          function<void(vector<T> &)> &&callBack) {
    _forEachPermutationImpl(vec, callBack, 0);
  }
  /// \brief Do not call this method manually! This is only used by void
  /// forEachPermutation(vector<T>&&, function<void(vector<T> &)> &&)
  template <typename T>
  void _forEachPermutationImpl(vector<T> &vec,
                               function<void(vector<T> &)> &callBack,
                               size_t n) {
    if (n == vec.size()) {
      callBack(vec);
    } else {
      for (size_t i = n; i < vec.size(); ++i) {
        swap(vec[i], vec[n]);
        _forEachPermutationImpl(vec, callBack, n + 1);
        swap(vec[i], vec[n]);
      }
    }
  }

public:
  /// \brief The start- and end-state of a regex-element
  using FSM_range = array<reference_wrapper<StateMachineNode>, 2>;
  /// \brief Initializes a new NFACreator, which is able to fill the
  /// StateMachine NFA with the information gained from the other parameters
  NFACreator(StateMachine &NFA, CrySLParser::OrderContext *order,
             CrySLParser::EventsContext *events, OrderConverter &orc)
      : order(order), events(events), NFA(NFA), orc(orc) {}
  /// \brief Creates a consecutive sequence of regex-elements in the NFA
  FSM_range createSequence(StateMachineNode &curr,
                           CrySLParser::OrderSequenceContext *seq);
  /// \brief the regex-element should occur zero, or one time
  void makeOptional(const FSM_range &rng) {
    rng[0].get().addTransition("", rng[1].get());
  }
  /// \brief the regex-element should occur zero, or more times
  void makeStar(FSM_range &&rng) {
    auto &last = NFA.addState();
    rng[1].get().addTransition("", rng[0].get());
    rng[0].get().addTransition("", last);
    rng[1] = last;
  }
  /// \brief the regex-element should occur one, or more times
  void makePlus(FSM_range &&rng) {
    rng[1].get().addTransition("", rng[0].get());
  }
  /// \brief apply the given unary operation (?, +, *) on the regex-element
  void makeUnary(FSM_range &&rng, char unary) {
    switch (unary) {
    case '?':
      makeOptional(rng);
      return;
    case '*':
      makeStar(move(rng));
      return;
    case '+':
      makePlus(move(rng));
      return;
    }
    throw logic_error("Invalid unary operation");
  }
  /// \brief Create a primary regex-element (event, or unary operation) in the
  /// NFA
  FSM_range createPrimary(StateMachineNode &curr,
                          CrySLParser::PrimaryContext *prim) {
    StateMachineNode *last;
    if (prim->eventName) {

      auto transitions = orc.getFunctionNames(prim->eventName->getText());
      if (transitions.size() == 1) {
        auto &nwNode = NFA.addState();
        last = &nwNode;
        curr.addTransition(*transitions.begin(), nwNode);
      } else {
        auto &res = NFA.addState();
        last = &res;
        for (auto &trn : transitions) {
          auto &nwNode = NFA.addState();
          curr.addTransition(trn, nwNode);
          nwNode.addTransition("", res);
        }
      }
    } else {
      auto firstlast = createSequence(curr, prim->orderSequence());
      last = &firstlast[1].get();
    }

    if (prim->elementop) {
      char op = prim->elementop->getText()[0];
      makeUnary({curr, *last}, op);
    }
    return {curr, *last};
  }
  /// \brief Creates an unordered sequence of regex-elements in the NFA
  ///
  /// Note: currently this consists of creating n! alternatives (one for each
  /// permutation of the sequence). This may change in the future.
  FSM_range createUnordered(StateMachineNode &curr,
                            CrySLParser::UnorderedSymbolsContext *uno) {
    auto prim = uno->primary();
    if (prim.size() == 1) {
      return createPrimary(curr, prim[0]);
    }

    long long lo, hi;
    long long max_hi = (long long)prim.size();
    if (uno->bound) {
      lo = uno->lower ? parseInt(uno->lower->getText()) : 0;
      hi = uno->upper ? parseInt(uno->upper->getText()) : max_hi;
    } else {
      hi = lo = max_hi;
    }
    // TODO do sth clever (I don't want to make n! alternatives...)
    vector<FSM_range> alts;
    alts.reserve(prim.size());
    for (auto p : prim) {
      alts.push_back(move(createPrimary(NFA.addState(), p)));
    }
    auto &last = NFA.addState();
    forEachPermutation<FSM_range>(
        move(alts), [hi, lo, &last, &curr, this](vector<FSM_range> &perm) {
          // make sequence for elements from 0 to lo
          // make optional sequence for elements from lo+1 to hi
          auto prev = &curr;
          for (size_t i = 0; i < lo; ++i) {
            prev->addTransition("", perm[i][0].get());
            prev = &perm[i][1].get();
          }
          for (size_t i = lo; i < hi; ++i) {
            auto opt = perm[i];
            makeOptional(opt);
            prev->addTransition("", opt[0].get());
            prev = &opt[1].get();
          }
          prev->addTransition("", last);
        });
    return {curr, last};
  }
  /// \brief Create alternative regex-elements in the NFA. One of them must
  /// occur
  FSM_range createAlternative(StateMachineNode &curr,
                              CrySLParser::SimpleOrderContext *alts) {
    auto &last = NFA.addState();
    for (auto uno : alts->unorderedSymbols()) {
      auto firstlast = createUnordered(curr, uno);
      firstlast[1].get().addTransition("", last);
    }
    return {curr, last};
  }
  /// \brief Performs the NFA-creation process
  void create() {
    auto firstlast =
        createSequence(NFA.getInitialState(), order->orderSequence());
    firstlast[1].get().addTransition("", NFA.getAcceptingState());
  }
};
NFACreator::FSM_range
NFACreator::createSequence(StateMachineNode &curr,
                           CrySLParser::OrderSequenceContext *seq) {
  auto nod = &curr;
  for (auto alt : seq->simpleOrder()) {
    auto firstlast = createAlternative(*nod, alt);
    nod = &firstlast[1].get();
  }
  return {curr, *nod};
}
unique_ptr<StateMachine>
OrderConverter::createFromContext(CrySLParser::OrderContext *order,
                                  CrySLParser::EventsContext *events) {
  auto ret = make_unique<StateMachine>();
  NFACreator nfc(*ret, order, events, *this);
  nfc.create();
  return ret;
}
} // namespace CCPP