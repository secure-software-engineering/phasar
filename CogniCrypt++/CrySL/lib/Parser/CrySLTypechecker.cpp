#include <CrySLTypechecker.h>
namespace CCPP {
// CrySL Typechecker

bool CrySLTypechecker::typecheck() {
  bool succ = true;
  std::vector<CrySLParser::EnsPredContext *> EnsuredPreds, NegatedPreds;
  std::vector<CrySLParser::ReqPredContext *> RequiredPreds;
  for (auto &ast : ASTs) {
    CrySLSpec spec(ast->getAST());
    if (succ = spec.typecheck()) {
      EnsuredPreds.insert(EnsuredPreds.end(), spec.ensuredPredicates().begin(),
                          spec.ensuredPredicates().end());
      NegatedPreds.insert(NegatedPreds.end(), spec.negatedPredicates().begin(),
                          spec.negatedPredicates().end());
      RequiredPreds.insert(RequiredPreds.end(),
                           spec.requiredPredicates().begin(),
                           spec.requiredPredicates().end());
    }
    if (succ) {
      succ &= typecheckEnsNegReq(EnsuredPreds, NegatedPreds, RequiredPreds);
    }
  }
  return succ;
}

CrySLTypechecker::CrySLTypechecker(
    std::vector<std::unique_ptr<ASTContext>> &ASTs)
    : ASTs(ASTs) {}
CrySLTypechecker::CrySLSpec::CrySLSpec(CrySLParser::DomainModelContext *AST)
    : AST(AST) {}
// CrySL Spec

bool CrySLTypechecker::CrySLSpec::typecheck() {
  std::cout << AST->getText() << std::endl;
  bool succ = true;
  succ &= typecheck(AST->objects());
  succ &= typecheck(AST->events());
  succ &= typecheck(AST->order());

  if (AST->forbidden())
    succ &= typecheck(AST->forbidden());
  if (AST->constraints())
    succ &= typecheck(AST->constraints());
  if (AST->requiresBlock())
    succ &= typecheck(AST->requiresBlock());
  if (AST->ensures())
    succ &= typecheck(AST->ensures());
  if (AST->negates())
    succ &= typecheck(AST->negates());

  return succ;
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