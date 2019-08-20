#pragma once

#include "CrySLParser.h"
#include "Types/Type.h"
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace CCPP {
using namespace Types;

class CrySLTypechecker {
  std::vector<CrySLParser::DomainModelContext *> &ASTs;

  class CrySLSpec {
    CrySLParser::DomainModelContext *AST;
    std::vector<CrySLParser::EnsPredContext *> EnsuredPreds, NegatedPreds;
    std::vector<CrySLParser::ReqPredContext *> RequiredPreds;
    // objects: name -> typename

    std::unordered_map<std::string, std::shared_ptr<Type>> DefinedObjects;
    // TODO other context objects;

    bool typecheck(CrySLParser::ObjectsContext *objs);
    bool typecheck(CrySLParser::ForbiddenContext *forb);
    bool typecheck(CrySLParser::EventsContext *evts);
    bool typecheck(CrySLParser::OrderContext *order);
    bool typecheck(CrySLParser::ConstraintsContext *constr);
    bool typecheck(CrySLParser::RequiresBlockContext *req);
    bool typecheck(CrySLParser::EnsuresContext *ens);
    bool typecheck(CrySLParser::NegatesContext *neg);

  public:
    CrySLSpec(CrySLParser::DomainModelContext *AST);
    bool typecheck();
    const std::vector<CrySLParser::EnsPredContext *> &ensuredPredicates() const;
    const std::vector<CrySLParser::ReqPredContext *> &
    requiredPredicates() const;
    const std::vector<CrySLParser::EnsPredContext *> &negatedPredicates() const;
  };
  bool
  typecheckEnsNegReq(std::vector<CrySLParser::EnsPredContext *> EnsuredPreds,
                     std::vector<CrySLParser::EnsPredContext *> NegatedPreds,
                     std::vector<CrySLParser::ReqPredContext *> RequiredPreds);

public:
  CrySLTypechecker(std::vector<CrySLParser::DomainModelContext *> &ASTs);
  bool typecheck();
};
} // namespace CCPP