#include "CrySLTypechecker.h"
#include "PositionHelper.h"
#include "Types/Type.h"
#include <iostream>

bool CCPP::CrySLTypechecker::CrySLSpec::checkPredicate(
    CrySLParser::EnsPredContext *ensu) {
  bool result = true;
  auto predFull = ensu->pred();
  result &= checkPredicate(predFull);
  if (ensu->state) {
    if (!DefinedEvents.count(ensu->Ident()->getText())) {
      result = false;
      std::cerr << Position(ensu->Ident())
                << ": The event is not defined in the EVENTS section"
                << std::endl;
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
            std::cerr << Position(idts)
                      << ": object is not defined in the OBJECTS section"
                      << std::endl;
            return false;
          }
        }
      }
    }
  }
  return true;
}

bool CCPP::CrySLTypechecker::CrySLSpec::typecheck(
    CrySLParser::EnsuresContext *ens) {
  bool result = true;

  for (auto ensu : ens->ensPred()) {
    EnsuredPreds.push_back(ensu);
    result &= checkPredicate(ensu);
  }
  return result;
}

bool CCPP::CrySLTypechecker::CrySLSpec::typecheck(
    CrySLParser::NegatesContext *neg) {
  bool result = true;

  for (auto ensu : neg->negatesOccurence()) {
    NegatedPreds.push_back(ensu->ensPred());
    result &= checkPredicate(ensu->ensPred());
  }
  return result;
}