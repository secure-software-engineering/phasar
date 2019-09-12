#include <Parser/CrySLTypechecker.h>
#include <Parser/ErrorHelper.h>
namespace CCPP {
// CrySL Typechecker

bool CrySLTypechecker::typecheck() {
  bool succ = true;
  std::vector<CrySLParser::EnsPredContext *> EnsuredPreds, NegatedPreds;
  std::vector<CrySLParser::ReqPredContext *> RequiredPreds;
  for (auto &ast : ASTs) {
    CrySLSpec spec(ast->getAST(), ast->getFilename());
    if (spec.typecheck()) {
      EnsuredPreds.insert(EnsuredPreds.end(), spec.ensuredPredicates().begin(),
                          spec.ensuredPredicates().end());
      NegatedPreds.insert(NegatedPreds.end(), spec.negatedPredicates().begin(),
                          spec.negatedPredicates().end());
      RequiredPreds.insert(RequiredPreds.end(),
                           spec.requiredPredicates().begin(),
                           spec.requiredPredicates().end());
    } else
      succ = false;
    errors |= spec.getErrors();
  }
  if (succ) {
    succ &= typecheckEnsNegReq(EnsuredPreds, NegatedPreds, RequiredPreds);
  }
  return succ;
}
bool CrySLTypechecker::interSpecificationTypecheck() {
  std::vector<CrySLParser::EnsPredContext *> EnsuredPreds, NegatedPreds;
  std::vector<CrySLParser::ReqPredContext *> RequiredPreds;
  for (auto &spec : specs) {
    EnsuredPreds.insert(EnsuredPreds.end(), spec.ensuredPredicates().begin(),
                        spec.ensuredPredicates().end());
    NegatedPreds.insert(NegatedPreds.end(), spec.negatedPredicates().begin(),
                        spec.negatedPredicates().end());
    RequiredPreds.insert(RequiredPreds.end(), spec.requiredPredicates().begin(),
                         spec.requiredPredicates().end());
  }
  return typecheckEnsNegReq(EnsuredPreds, NegatedPreds, RequiredPreds);
}
CrySLTypechecker::CrySLTypechecker(
    std::vector<std::unique_ptr<ASTContext>> &ASTs)
    : ASTs(ASTs) {}
CrySLTypechecker::CrySLTypechecker(
    std::vector<std::unique_ptr<ASTContext>> &ASTs,
    std::vector<CrySLSpec> &&specs)
    : ASTs(ASTs), specs(std::move(specs)) {}

CrySLTypechecker::TypeCheckKind CrySLTypechecker::getErrors() const {
  return (TypeCheckKind)errors;
}
CrySLTypechecker::TypeCheckKind CrySLTypechecker::CrySLSpec::getErrors() const {
  return (TypeCheckKind)errors;
}

CrySLTypechecker::CrySLSpec::CrySLSpec(CrySLParser::DomainModelContext *AST,
                                       const std::string &filename)
    : AST(AST), filename(filename) {}
// CrySL Spec

bool CrySLTypechecker::CrySLSpec::typecheck() {
  // std::cout << AST->getText() << std::endl;

  if (!typecheck(AST->objects()))
    errors |= OBJECTS;

  if (!typecheck(AST->events()))
    errors |= EVENTS;

  std::unordered_map<std::string, std::unordered_set<std::string>> UnusedEvents(
      DefinedEvents);

  if (!typecheck(AST->order(), UnusedEvents))
    errors |= ORDER;

  if (AST->forbidden() && !typecheck(AST->forbidden()))
    errors |= FORBIDDEN;
  if (AST->constraints() && !typecheck(AST->constraints()))
    errors |= CONSTRAINTS;
  if (AST->requiresBlock() && !typecheck(AST->requiresBlock()))
    errors |= REQUIRES;
  if (AST->ensures() && !typecheck(AST->ensures(), UnusedEvents))
    errors |= ENSURES;
  if (AST->negates() && !typecheck(AST->negates(), UnusedEvents))
    errors |= NEGATES;

  if (!UnusedEvents.empty()) {
    // std::cerr << "Warning: " << Position(order, filename) << ": events {";
    std::stringstream ss;
    bool first = true;
    for (auto &evt : UnusedEvents) {
      if (!first)
        ss << ", ";
      ss << evt.first;
      first = false;
    }
    // std::cerr << "} are defined in EVENTS section but not called in ORDER "
    //             "section"
    //          << std::endl;
    reportWarning(
        Position(AST->events(), filename),
        {"The events {", ss.str(),
         "} are defined in EVENTS section but not called in ORDER section"});
  }
  return errors == NONE;
}

const std::vector<CrySLParser::EnsPredContext *> &
CrySLTypechecker::CrySLSpec::ensuredPredicates() const {
  return this->EnsuredPreds;
}
const std::vector<CrySLParser::ReqPredContext *> &
CrySLTypechecker::CrySLSpec::requiredPredicates() const {
  return this->RequiredPreds;
}
const std::vector<CrySLParser::EnsPredContext *> &
CrySLTypechecker::CrySLSpec::negatedPredicates() const {
  return this->NegatedPreds;
}
} // namespace CCPP