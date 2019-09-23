#pragma once
#include <FormatConverter/DFA/DFA.h>
#include <memory>
#include <phasar/PhasarLLVM/IfdsIde/Problems/TypeStateDescriptions/TypeStateDescription.h>
#include <string>
#include <unordered_map>

namespace CCPP {
class OrderTypeStateDescription : public psr::TypeStateDescription {
  std::unique_ptr<DFA::DFA> dfa;
  std::unordered_map<std::string, int> eventTransitions;
  std::string typeName;

public:
  /// \brief Construct a OrderTypeStateDescription
  /// TODO: parameters
  OrderTypeStateDescription(
      const std::string &typeName, std::unique_ptr<DFA::DFA> &&dfa,
      std::unordered_map<std::string, int> &&eventTransitions);
  /// \brief True, iff F is a function, which returns an object of the
  /// getTypeNameOfInterest() type
  virtual bool isFactoryFunction(const std::string &F) const override;
  /// \brief True, iff an object of the getTypeNameOfInterest() type can be
  /// passed to F as a parameter
  virtual bool isConsumingFunction(const std::string &F) const override;
  /// \brief True, iff F is a function from the API of the
  /// getTypeNameOfInterest() type
  virtual bool isAPIFunction(const std::string &F) const override;
  /// \brief Implements the delta-function of the DFA
  virtual State getNextState(std::string Tok, State S) const override;
  /// \brief The name of the Type of objects to track in the TypeStateAnalysis
  virtual std::string getTypeNameOfInterest() const override;
  /// \brief The parameter-indices of F which consume an object of
  /// getTypeNameOfInterest() type
  virtual std::set<int>
  getConsumerParamIdx(const std::string &F) const override;
  /// \brief The parameter-indices of F, which return a new object of
  /// getTypeNameOfInterest() type; The return-value has index -1
  virtual std::set<int> getFactoryParamIdx(const std::string &F) const override;
  /// \brief Converts the given State S to its string-representation
  virtual std::string stateToString(State S) const override;
  /// \brief The bottom-state representing "all information"
  virtual State bottom() const override;
  /// \brief The top-state representing "no information"
  virtual State top() const override;
  /// \brief The starting state before initialization of the object
  virtual State uninit() const override;
  /// \brief The starting state after initialization of the object
  virtual State start() const override;
  /// \brief The error-state
  virtual State error() const override;
  /// \brief the accepting final state
  bool isAccepting(State S) const;
};
} // namespace CCPP