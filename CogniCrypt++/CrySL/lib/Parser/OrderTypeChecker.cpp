#include <CrySLTypechecker.h>
#include <ErrorHelper.h>
#include <TokenHelper.h>
#include <TypeParser.h>
#include <Types/Type.h>
#include <iostream>
#include <sstream>

namespace CCPP {
// using namespace std;

void markEventAsCalled(
    const std::string &evt,
    std::unordered_map<std::string, std::unordered_set<std::string>>
        &DefinedEventsDummy) {
  auto found = DefinedEventsDummy.find(evt);
  if (found != DefinedEventsDummy.end()) {
    if (found->second.size() > 1) {
      auto aggregates = std::move(found->second);
      DefinedEventsDummy.erase(evt);
      for (auto &agg : aggregates) {
        markEventAsCalled(agg, DefinedEventsDummy);
      }
    } else {
      DefinedEventsDummy.erase(evt);
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
      reportError(Position(uo->upper, filename),
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
        &DefinedEventsDummy,
    const std::unordered_map<std::string, std::unordered_set<std::string>>
        &DefinedEvents,
    const std::string &filename) {

  const auto name = primaryContext->eventName->getText();
  // DefinedEventsDummy.erase(name);
  markEventAsCalled(name, DefinedEventsDummy);
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
        &DefinedEventsDummy,
    const std::unordered_map<std::string, std::unordered_set<std::string>>
        &DefinedEvents,
    const std::string &filename) {
  bool result = true;
  for (auto simpleOrder : seq->simpleOrder()) {
    for (auto unorderedSymbolsContext : simpleOrder->unorderedSymbols()) {
      result &= checkBound(unorderedSymbolsContext, filename);
      for (auto primaryContext : unorderedSymbolsContext->primary()) {
        if (primaryContext->eventName)
          result &= checkEvent(primaryContext, DefinedEventsDummy,
                               DefinedEvents, filename);
        else {
          result &=
              checkOrderSequence(primaryContext->orderSequence(),
                                 DefinedEventsDummy, DefinedEvents, filename);
        }
      }
    }
  }
  return result;
}

bool CrySLTypechecker::CrySLSpec::typecheck(CrySLParser::OrderContext *order) {

  std::unordered_map<std::string, std::unordered_set<std::string>>
      DefinedEventsDummy(DefinedEvents);
  bool result = checkOrderSequence(order->orderSequence(), DefinedEventsDummy,
                                   DefinedEvents, filename);
  if (!DefinedEventsDummy.empty()) {
    // std::cerr << "Warning: " << Position(order, filename) << ": events {";
    std::stringstream ss;
    bool first = true;
    for (auto &evt : DefinedEventsDummy) {
      if (!first)
        ss << ", ";
      ss << evt.first;
      first = false;
    }
    // std::cerr << "} are defined in EVENTS section but not called in ORDER "
    //             "section"
    //          << std::endl;
    reportWarning(
        Position(order, filename),
        {"The events {", ss.str(),
         "} are defined in EVENTS section but not called in ORDER section"});
  }
  return result;
}

} // namespace CCPP