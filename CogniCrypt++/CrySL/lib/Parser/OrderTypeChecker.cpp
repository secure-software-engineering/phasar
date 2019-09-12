#include <Parser/CrySLTypechecker.h>
#include <Parser/ErrorHelper.h>
#include <Parser/TokenHelper.h>
#include <Parser/TypeParser.h>
#include <Parser/Types/Type.h>
#include <iostream>
#include <sstream>

namespace CCPP {
// using namespace std;

void CrySLTypechecker::CrySLSpec::markEventAsCalled(
    const std::string &evt,
    std::unordered_map<std::string, std::unordered_set<std::string>>
        &UnusedEvents) {
  auto found = UnusedEvents.find(evt);
  if (found != UnusedEvents.end()) {
    if (found->second.size() > 1) {
      auto aggregates = std::move(found->second);
      UnusedEvents.erase(evt);
      for (auto &agg : aggregates) {
        markEventAsCalled(agg, UnusedEvents);
      }
    } else {
      UnusedEvents.erase(evt);
    }
  }
}

bool checkBound(CrySLParser::UnorderedSymbolsContext *uno,
                const std::string &filename) {
  bool succ = true;
  if (uno->bound) {
    long long lo, hi;
    long long max_hi = (long long)uno->primary().size();
    lo = uno->lower ? parseInt(uno->lower->getText()) : 0;
    hi = uno->upper ? parseInt(uno->upper->getText()) : max_hi;

    if (lo > hi) {
      // std::cerr << Position(uno->bound, filename)
      //          << ": The lower bound must not be greater than the upper
      //          bound"
      //          << std::endl;
      reportError(Position(uno->bound, filename),
                  {"The lower bound ", std::to_string(lo),
                   " must not be larger then the upper bound ",
                   std::to_string(hi), "."});
      succ = false;
    }
    if (hi > max_hi) {
      // std::cerr << Position(uno->upper, filename)
      //          << ": The upper bound must not be greater than the number of "
      //             "primary expressions in the unordered set"
      //          << std::endl;
      reportError(Position(uno->upper, filename),
                  {"The upper bound ", std::to_string(hi),
                   " must not be larger then the number of primary expressions "
                   "in the unordered set, which is ",
                   std::to_string(max_hi)});
      succ = false;
    }
  }
  return succ;
}

bool checkEvent(
    CrySLParser::PrimaryContext *primaryContext,
    std::unordered_map<std::string, std::unordered_set<std::string>>
        &UnusedEvents,
    const std::unordered_map<std::string, std::unordered_set<std::string>>
        &DefinedEvents,
    const std::string &filename) {

  const auto name = primaryContext->eventName->getText();
  // UnusedEvents.erase(name);
  CrySLTypechecker::CrySLSpec::markEventAsCalled(name, UnusedEvents);
  // std::cout << ":::Name: " << name << std::endl;
  if (!DefinedEvents.count(name)) {
    // std::cerr << Position(primaryContext, filename) << ": event '" << name
    //          << "' is not defined in EVENTS section but is called in "
    //             "ORDER section"
    //          << std::endl;
    reportError(
        Position(primaryContext, filename),
        {"The event '", name, "' is not defined in the EVENTS section"});
    return false;
  }
  return true;
}

bool checkOrderSequence(
    CrySLParser::OrderSequenceContext *seq,
    std::unordered_map<std::string, std::unordered_set<std::string>>
        &UnusedEvents,
    const std::unordered_map<std::string, std::unordered_set<std::string>>
        &DefinedEvents,
    const std::string &filename) {
  bool result = true;
  for (auto simpleOrder : seq->simpleOrder()) {
    for (auto unorderedSymbolsContext : simpleOrder->unorderedSymbols()) {
      result &= checkBound(unorderedSymbolsContext, filename);
      for (auto primaryContext : unorderedSymbolsContext->primary()) {
        if (primaryContext->eventName)
          result &=
              checkEvent(primaryContext, UnusedEvents, DefinedEvents, filename);
        else {
          result &= checkOrderSequence(primaryContext->orderSequence(),
                                       UnusedEvents, DefinedEvents, filename);
        }
      }
    }
  }
  return result;
}

bool CrySLTypechecker::CrySLSpec::typecheck(
    CrySLParser::OrderContext *order,
    std::unordered_map<std::string, std::unordered_set<std::string>>
        &UnusedEvents) {

  bool result = checkOrderSequence(order->orderSequence(), UnusedEvents,
                                   DefinedEvents, filename);

  return result;
}

} // namespace CCPP