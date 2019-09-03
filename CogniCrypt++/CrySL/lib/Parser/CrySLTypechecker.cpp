#include <CrySLTypechecker.h>
namespace CCPP {
// CrySL Typechecker

bool CrySLTypechecker::typecheck() {
  bool succ = true;
  std::vector<CrySLParser::EnsPredContext *> EnsuredPreds, NegatedPreds;
  std::vector<CrySLParser::ReqPredContext *> RequiredPreds;
  for (auto &ast : ASTs) {
    CrySLSpec spec(ast->getAST(), ast->getFilename());
    if (succ = spec.typecheck()) {
      EnsuredPreds.insert(EnsuredPreds.end(), spec.ensuredPredicates().begin(),
                          spec.ensuredPredicates().end());
      NegatedPreds.insert(NegatedPreds.end(), spec.negatedPredicates().begin(),
                          spec.negatedPredicates().end());
      RequiredPreds.insert(RequiredPreds.end(),
                           spec.requiredPredicates().begin(),
                           spec.requiredPredicates().end());
    }
    errors |= spec.getErrors();
    if (succ) {
      succ &= typecheckEnsNegReq(EnsuredPreds, NegatedPreds, RequiredPreds);
    }
  }
  return succ;
}

CrySLTypechecker::CrySLTypechecker(
    std::vector<std::unique_ptr<ASTContext>> &ASTs)
    : ASTs(ASTs) {}

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

  if (!typecheck(AST->order()))
    errors |= ORDER;

  if (AST->forbidden() && !typecheck(AST->forbidden()))
    errors |= FORBIDDEN;
  if (AST->constraints() && !typecheck(AST->constraints()))
    errors |= CONSTRAINTS;
  if (AST->requiresBlock() && !typecheck(AST->requiresBlock()))
    errors |= REQUIRES;
  if (AST->ensures() && !typecheck(AST->ensures()))
    errors |= ENSURES;
  if (AST->negates() && !typecheck(AST->negates()))
    errors |= NEGATES;

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