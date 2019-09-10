#include <NFACreator.h>
#include <array>
#include <functional>
#include <stdexcept>
#include <unordered_set>
#include <utility>
#include <vector>

namespace CCPP {
using namespace std;
using namespace DFA;
class NFACreator {
  CrySLParser::OrderContext *order;
  CrySLParser::EventsContext *events;
  StateMachine &NFA;
  OrderConverter &orc;

public:
  using FSM_range = array<reference_wrapper<StateMachineNode>, 2>;
  NFACreator(StateMachine &NFA, CrySLParser::OrderContext *order,
             CrySLParser::EventsContext *events, OrderConverter &orc)
      : order(order), events(events), NFA(NFA), orc(orc) {}
  FSM_range createSequence(StateMachineNode &curr,
                           CrySLParser::OrderSequenceContext *seq);
  FSM_range createOptional(const FSM_range &rng) {}
  FSM_range createStar(const FSM_range &rng) {}
  FSM_range createPlus(const FSM_range &rng) {}
  FSM_range createUnary(const FSM_range &rng, char unary) {
    switch (unary) {
    case '?':
      return createOptional(rng);
    case '*':
      return createStar(rng);
    case '+':
      return createPlus(rng);
    }
    throw logic_error("Invalid unary operation");
  }
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

    if(prim->elementop){
      char op = prim->elementop->getText()[0];
      return createUnary({curr, *last}, op);
    }
    return {curr, *last};
  }
  FSM_range createUnordered(StateMachineNode &curr,
                            CrySLParser::UnorderedSymbolsContext *uno) {
    // TODO do sth clever (I don't want to make n! alternatives...)
  }
  FSM_range createAlternative(StateMachineNode &curr,
                              CrySLParser::SimpleOrderContext *alts) {
    auto &last = NFA.addState();
    for (auto uno : alts->unorderedSymbols()) {
      auto firstlast = createUnordered(curr, uno);
      firstlast[1].get().addTransition("", last);
    }
    return {curr, last};
  }
  FSM_range createSequence(StateMachineNode &curr,
                           CrySLParser::OrderSequenceContext *seq) {
    auto nod = &curr;
    for (auto alt : seq->simpleOrder()) {
      auto firstlast = createAlternative(*nod, alt);
      nod = &firstlast[1].get();
    }
    return {curr, *nod};
  }
  void create() {
    auto firstlast =
        createSequence(NFA.getInitialState(), order->orderSequence());
    firstlast[1].get().addTransition("", NFA.getAcceptingState());
  }
};

unique_ptr<StateMachine>
OrderConverter::createFromContext(CrySLParser::OrderContext *order,
                                  CrySLParser::EventsContext *events) {
  auto ret = make_unique<DFA::StateMachine>();
  NFACreator nfc(*ret, order, events, *this);
  nfc.create();
  return ret;
}
} // namespace CCPP