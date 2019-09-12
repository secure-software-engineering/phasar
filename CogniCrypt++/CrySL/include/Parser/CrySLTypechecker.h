#pragma once

#include <Parser/CrySLParser.h>
#include <Parser/Types/Type.h>
#include <Parser/ASTContext.h>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>
namespace CCPP {
using namespace Types;
/// \brief This is the core class for typechecking CrySL specifications
class CrySLTypechecker {
public:
  enum TypeCheckKind : int {
    NONE = 0,
    ORDER = 1,
    OBJECTS = 2,
    EVENTS = 4,
    ENSURES = 8,
    NEGATES = 16,
    REQUIRES = 32,
    FORBIDDEN = 64,
    CONSTRAINTS = 128,
  };
  /// \brief This is the core class for typechecking the parts of a single CrySL
  /// specification, which are independent from other specifications
  class CrySLSpec {
    CrySLParser::DomainModelContext *AST;
    const std::string &filename;
    std::vector<CrySLParser::EnsPredContext *> EnsuredPreds, NegatedPreds;
    std::vector<CrySLParser::ReqPredContext *> RequiredPreds;
    int errors = NONE;

    // objects: name -> typename
    std::unordered_map<std::string, std::shared_ptr<Type>> DefinedObjects;
    // eventName -> aggregates (for non-aggregate events: identity)
    std::unordered_map<std::string, std::unordered_set<std::string>>
        DefinedEvents;

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
    /// \param UnusedEvents The events that are currently defined, but not used.
    /// \return True, iff the spec is semantically correct
    bool
    typecheck(CrySLParser::OrderContext *order,
              std::unordered_map<std::string, std::unordered_set<std::string>>
                  &UnusedEvents);
    /// \brief Helper method for typechecking the CONSTRAINTS section of a CrySL
    /// spec
    ///
    /// \param constr The AST node representing the CONSTRAINTS section
    /// \return True, iff the spec is semantically correct
    bool typecheck(CrySLParser::ConstraintsContext *constr);

    bool checkPredicate(CrySLParser::ReqPredContext *requ);
    bool checkPredicate(
        CrySLParser::EnsPredContext *ensu,
        std::unordered_map<std::string, std::unordered_set<std::string>>
            &UnusedEvents);
    bool checkPredicate(CrySLParser::PredContext *pred);

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
    /// \param UnusedEvents The events that are currently defined, but not used.
    /// \return True, iff the spec is semantically correct
    bool
    typecheck(CrySLParser::EnsuresContext *ens,
              std::unordered_map<std::string, std::unordered_set<std::string>>
                  &UnusedEvents);
    /// \brief Helper method for typechecking the NEGATES section of a CrySL
    /// spec
    ///
    /// \param neg The AST node representing the NEGATES section
    /// \param UnusedEvents The events that are currently defined, but not used.
    /// \return True, iff the spec is semantically correct
    bool
    typecheck(CrySLParser::NegatesContext *neg,
              std::unordered_map<std::string, std::unordered_set<std::string>>
                  &UnusedEvents);

  public:
    CrySLSpec(CrySLParser::DomainModelContext *AST,
              const std::string &filename);
    /// \brief Marks the event evt as "called somewhere" by removing it and all
    /// dependencies from DefinedEventsDummy
    static void markEventAsCalled(
        const std::string &evt,
        std::unordered_map<std::string, std::unordered_set<std::string>>
            &UnusedEvents);
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
    TypeCheckKind getErrors() const;
  };

private:
  int errors = NONE;
  std::vector<std::unique_ptr<ASTContext>> &ASTs;

  std::vector<CrySLSpec> specs;
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
  CrySLTypechecker(std::vector<std::unique_ptr<ASTContext>> &ASTs);
  CrySLTypechecker(std::vector<std::unique_ptr<ASTContext>> &ASTs,
                   std::vector<CrySLSpec> &&specs);
  /// \brief Performs the typechecking for all CrySL specs, which have been
  /// passed to the constructor
  bool typecheck();
  /// \brief Performs only the inter-spec typechecks assuming that the other
  /// typechecks are already done.
  ///
  /// Only use this function, when this object is constructed with the second
  /// constructor (which also receives the CrySLSpec-vector)
  bool interSpecificationTypecheck();
  TypeCheckKind getErrors() const;
};
} // namespace CCPP