#include <Parser/CrySLTypechecker.h>
#include <Parser/PositionHelper.h>
#include <Parser/Types/Type.h>
#include <Parser/ErrorHelper.h>
#include <iostream>

bool CCPP::CrySLTypechecker::CrySLSpec::checkPredicate(
    CrySLParser::EnsPredContext *ensu,
    std::unordered_map<std::string, std::unordered_set<std::string>>
        &UnusedEvents) {
  bool result = true;
  auto predFull = ensu->pred();
  result &= checkPredicate(predFull);
  if (ensu->state) {
    if (!DefinedEvents.count(ensu->Ident()->getText())) {
      result = false;
      // std::cerr << Position(ensu->Ident(), filename)
      //          << ": The event is not defined in the EVENTS section"
      //          << std::endl;
      reportError(Position(ensu->Ident()),
                  "The event is not defined in the EVENTS section");
    } else {
      markEventAsCalled(ensu->Ident()->getText(), UnusedEvents);
    }
  }
  return true;
}
bool CCPP::CrySLTypechecker::CrySLSpec::checkPredicate(
    CrySLParser::PredContext *predFull) {
  if (predFull->suParList()) {
    auto parameters = predFull->suParList()->suPar();
    for (auto perP : parameters) {
      if (perP->consPred() && perP->consPred()->literalExpr() &&
          perP->consPred()->literalExpr()->memberAccess()) {
        for (auto idts :
             perP->consPred()->literalExpr()->memberAccess()->Ident()) {
          if (!DefinedObjects.count(idts->getText())) {
            // std::cerr << Position(idts, filename)
            //          << ": object is not defined in the OBJECTS section"
            //          << std::endl;
            reportError(Position(idts, filename),
                        {"The object '", idts->getText(),
                         "' is not defined in the OBJECTS section"});
            return false;
          }
        }
      }
    }
  }
  return true;
}

bool CCPP::CrySLTypechecker::CrySLSpec::typecheck(
    CrySLParser::EnsuresContext *ens,
    std::unordered_map<std::string, std::unordered_set<std::string>>
        &UnusedEvents) {
  bool result = true;

  for (auto ensu : ens->ensPred()) {
    EnsuredPreds.push_back(ensu);
    result &= checkPredicate(ensu, UnusedEvents);
  }
  return result;
}

bool CCPP::CrySLTypechecker::CrySLSpec::typecheck(
    CrySLParser::NegatesContext *neg,
    std::unordered_map<std::string, std::unordered_set<std::string>>
        &UnusedEvents) {
  bool result = true;

  for (auto ensu : neg->negatesOccurence()) {
    NegatedPreds.push_back(ensu->ensPred());
    result &= checkPredicate(ensu->ensPred(), UnusedEvents);
  }
  return result;
}