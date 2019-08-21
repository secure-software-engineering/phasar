#pragma once

#include "CrySLParser.h"
#include "Types/Type.h"
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace CCPP {
using namespace Types;
/// \brief This is the core class for typechecking CrySL specifications
class CrySLTypechecker {
  std::vector<CrySLParser::DomainModelContext *> &ASTs;
  /// \brief This is the core class for typechecking the parts of a single CrySL
  /// specification, which are independent from other specifications
  class CrySLSpec {
    CrySLParser::DomainModelContext *AST;
    std::vector<CrySLParser::EnsPredContext *> EnsuredPreds, NegatedPreds;
    std::vector<CrySLParser::ReqPredContext *> RequiredPreds;

    // objects: name -> typename
    std::unordered_map<std::string, std::shared_ptr<Type>> DefinedObjects;
    std::unordered_set<std::string> DefinedEvents;
    // TODO other context objects;

    /// \brief Helper method for typechecking the OBJECTS section of a CrySL
    /// spec
    ///
    /// \param objs The AST node representing the OBJECTS section \return
    /// True, iff the spec is semantically correct
    bool typecheck(CrySLParser::ObjectsContext *objs);
    /// \brief Helper method for typechecking the FORBIDDEN section of a CrySL
    /// spec
    ///
    /// \param forb The AST node representing the FORBIDDEN section \return
    /// True, iff the spec is semantically correct
    bool typecheck(CrySLParser::ForbiddenContext *forb);
    /// \brief Helper method for typechecking the EVENTS section of a CrySL spec
    ///
    /// \param evts The AST node representing the EVENTS section
    /// \return True, iff the spec is semantically correct
    bool typecheck(CrySLParser::EventsContext *evts);
    /// \brief Helper method for typechecking the ORDER section of a CrySL spec
    ///
    /// \param order The AST node representing the ORDER section
    /// \return True, iff the spec is semantically correct
    bool typecheck(CrySLParser::OrderContext *order);
    /// \brief Helper method for typechecking the CONSTRAINTS section of a CrySL
    /// spec
    ///
    /// \param constr The AST node representing the CONSTRAINTS section
    /// \return True, iff the spec is semantically correct
    bool typecheck(CrySLParser::ConstraintsContext *constr);
    /// \brief Helper method for typechecking the REQUIRES section of a CrySL
    /// spec
    ///
    /// \param req The AST node representing the REQUIRES section
    /// \return True, iff the spec is semantically correct
    bool typecheck(CrySLParser::RequiresBlockContext *req);
    /// \brief Helper method for typechecking the ENSURES section of a CrySL
    /// spec
    ///
    /// \param ens The AST node representing the ENSURES section
    /// \return True, iff the spec is semantically correct
    bool typecheck(CrySLParser::EnsuresContext *ens);
    /// \brief Helper method for typechecking the NEGATES section of a CrySL
    /// spec
    ///
    /// \param neg The AST node representing the NEGATES section
    /// \return True, iff the spec is semantically correct
    bool typecheck(CrySLParser::NegatesContext *neg);

  public:
    CrySLSpec(CrySLParser::DomainModelContext *AST);
    /// \brief Performs the typechecking for the CrySL spec, which is passed to
    /// the constructor
    bool typecheck();
    /// \brief A collection of all ensured predicates
    const std::vector<CrySLParser::EnsPredContext *> &ensuredPredicates() const;
    /// \brief A collection of all required predicates
    const std::vector<CrySLParser::ReqPredContext *> &
    requiredPredicates() const;
    /// \brief A collection of all negated predicates
    const std::vector<CrySLParser::EnsPredContext *> &negatedPredicates() const;
  };
  /// \brief A helper method for "type"checking the parts of the CrySL
  /// specifications, which are dependent of other specs
  ///
  /// Concretely checks whether required/negated predicates can be ensured at
  /// all
  /// \param EnsuredPreds A collection of all ensured predicates of all CrySL
  /// specs involved.
  /// \param NegatedPreds A collection of all negated predicates of all CrySL
  /// specs involved.
  /// \param RequiredPreds A collection of all required predicates of all CrySL
  /// specs involved.
  bool
  typecheckEnsNegReq(std::vector<CrySLParser::EnsPredContext *> EnsuredPreds,
                     std::vector<CrySLParser::EnsPredContext *> NegatedPreds,
                     std::vector<CrySLParser::ReqPredContext *> RequiredPreds);

public:
  CrySLTypechecker(std::vector<CrySLParser::DomainModelContext *> &ASTs);
  /// \brief Performs the typechecking for all CrySL specs, which have been
  /// passed to the constructor
  bool typecheck();
};
} // namespace CCPP