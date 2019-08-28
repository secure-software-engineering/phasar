
// Generated from .\CrySL.g4 by ANTLR 4.7.2

#pragma once


#include "antlr4-runtime.h"




class  CrySLParser : public antlr4::Parser {
public:
  enum {
    T__0 = 1, T__1 = 2, T__2 = 3, T__3 = 4, T__4 = 5, T__5 = 6, T__6 = 7, 
    T__7 = 8, T__8 = 9, T__9 = 10, T__10 = 11, T__11 = 12, T__12 = 13, T__13 = 14, 
    T__14 = 15, T__15 = 16, T__16 = 17, T__17 = 18, T__18 = 19, T__19 = 20, 
    T__20 = 21, T__21 = 22, T__22 = 23, T__23 = 24, T__24 = 25, T__25 = 26, 
    T__26 = 27, T__27 = 28, T__28 = 29, T__29 = 30, T__30 = 31, T__31 = 32, 
    T__32 = 33, T__33 = 34, T__34 = 35, T__35 = 36, T__36 = 37, T__37 = 38, 
    T__38 = 39, T__39 = 40, T__40 = 41, T__41 = 42, T__42 = 43, T__43 = 44, 
    T__44 = 45, T__45 = 46, T__46 = 47, T__47 = 48, T__48 = 49, T__49 = 50, 
    T__50 = 51, T__51 = 52, T__52 = 53, T__53 = 54, T__54 = 55, T__55 = 56, 
    T__56 = 57, T__57 = 58, T__58 = 59, T__59 = 60, T__60 = 61, T__61 = 62, 
    T__62 = 63, T__63 = 64, Int = 65, Double = 66, Char = 67, Bool = 68, 
    String = 69, Ident = 70, COMMENT = 71, LINE_COMMENT = 72, WS = 73
  };

  enum {
    RuleDomainModel = 0, RuleSpec = 1, RuleQualifiedName = 2, RuleObjects = 3, 
    RuleObjectDecl = 4, RulePrimitiveTypeName = 5, RuleTypeName = 6, RulePtr = 7, 
    RuleArray = 8, RuleRequiresBlock = 9, RuleReqPred = 10, RuleReqPredLit = 11, 
    RulePred = 12, RuleSuParList = 13, RuleSuPar = 14, RuleConsPred = 15, 
    RuleLiteralExpr = 16, RuleLiteral = 17, RuleMemberAccess = 18, RulePreDefinedPredicate = 19, 
    RuleEnsures = 20, RuleEnsPred = 21, RuleConstraints = 22, RuleConstr = 23, 
    RuleComparingRelOperator = 24, RuleCons = 25, RuleArrayElements = 26, 
    RuleLitList = 27, RuleEvents = 28, RuleEventsOccurence = 29, RuleParametersList = 30, 
    RuleParam = 31, RuleEventAggregate = 32, RuleAgg = 33, RuleOrder = 34, 
    RuleOrderSequence = 35, RuleSimpleOrder = 36, RuleUnorderedSymbols = 37, 
    RulePrimary = 38, RuleNegates = 39, RuleNegatesOccurence = 40, RuleForbidden = 41, 
    RuleForbiddenOccurence = 42, RuleFqn = 43, RuleTypeNameList = 44
  };

  CrySLParser(antlr4::TokenStream *input);
  ~CrySLParser();

  virtual std::string getGrammarFileName() const override;
  virtual const antlr4::atn::ATN& getATN() const override { return _atn; };
  virtual const std::vector<std::string>& getTokenNames() const override { return _tokenNames; }; // deprecated: use vocabulary instead.
  virtual const std::vector<std::string>& getRuleNames() const override;
  virtual antlr4::dfa::Vocabulary& getVocabulary() const override;


  class DomainModelContext;
  class SpecContext;
  class QualifiedNameContext;
  class ObjectsContext;
  class ObjectDeclContext;
  class PrimitiveTypeNameContext;
  class TypeNameContext;
  class PtrContext;
  class ArrayContext;
  class RequiresBlockContext;
  class ReqPredContext;
  class ReqPredLitContext;
  class PredContext;
  class SuParListContext;
  class SuParContext;
  class ConsPredContext;
  class LiteralExprContext;
  class LiteralContext;
  class MemberAccessContext;
  class PreDefinedPredicateContext;
  class EnsuresContext;
  class EnsPredContext;
  class ConstraintsContext;
  class ConstrContext;
  class ComparingRelOperatorContext;
  class ConsContext;
  class ArrayElementsContext;
  class LitListContext;
  class EventsContext;
  class EventsOccurenceContext;
  class ParametersListContext;
  class ParamContext;
  class EventAggregateContext;
  class AggContext;
  class OrderContext;
  class OrderSequenceContext;
  class SimpleOrderContext;
  class UnorderedSymbolsContext;
  class PrimaryContext;
  class NegatesContext;
  class NegatesOccurenceContext;
  class ForbiddenContext;
  class ForbiddenOccurenceContext;
  class FqnContext;
  class TypeNameListContext; 

  class  DomainModelContext : public antlr4::ParserRuleContext {
  public:
    DomainModelContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    SpecContext *spec();
    ObjectsContext *objects();
    EventsContext *events();
    OrderContext *order();
    antlr4::tree::TerminalNode *EOF();
    ForbiddenContext *forbidden();
    ConstraintsContext *constraints();
    RequiresBlockContext *requiresBlock();
    EnsuresContext *ensures();
    NegatesContext *negates();

   
  };

  DomainModelContext* domainModel();

  class  SpecContext : public antlr4::ParserRuleContext {
  public:
    SpecContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    QualifiedNameContext *qualifiedName();

   
  };

  SpecContext* spec();

  class  QualifiedNameContext : public antlr4::ParserRuleContext {
  public:
    QualifiedNameContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<antlr4::tree::TerminalNode *> Ident();
    antlr4::tree::TerminalNode* Ident(size_t i);

   
  };

  QualifiedNameContext* qualifiedName();

  class  ObjectsContext : public antlr4::ParserRuleContext {
  public:
    ObjectsContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<ObjectDeclContext *> objectDecl();
    ObjectDeclContext* objectDecl(size_t i);

   
  };

  ObjectsContext* objects();

  class  ObjectDeclContext : public antlr4::ParserRuleContext {
  public:
    antlr4::Token *constModifier = nullptr;;
    ObjectDeclContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    TypeNameContext *typeName();
    antlr4::tree::TerminalNode *Ident();
    std::vector<ArrayContext *> array();
    ArrayContext* array(size_t i);

   
  };

  ObjectDeclContext* objectDecl();

  class  PrimitiveTypeNameContext : public antlr4::ParserRuleContext {
  public:
    antlr4::Token *booleanType = nullptr;;
    antlr4::Token *unsignedInt = nullptr;;
    antlr4::Token *charTy = nullptr;;
    antlr4::Token *shortTy = nullptr;;
    antlr4::Token *intTy = nullptr;;
    antlr4::Token *longTy = nullptr;;
    antlr4::Token *longlongTy = nullptr;;
    antlr4::Token *floatingPoint = nullptr;;
    antlr4::Token *longDouble = nullptr;;
    antlr4::Token *doubleFloat = nullptr;;
    antlr4::Token *sizeType = nullptr;;
    PrimitiveTypeNameContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;

   
  };

  PrimitiveTypeNameContext* primitiveTypeName();

  class  TypeNameContext : public antlr4::ParserRuleContext {
  public:
    CrySLParser::PtrContext *pointer = nullptr;;
    TypeNameContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    QualifiedNameContext *qualifiedName();
    PrimitiveTypeNameContext *primitiveTypeName();
    std::vector<PtrContext *> ptr();
    PtrContext* ptr(size_t i);

   
  };

  TypeNameContext* typeName();

  class  PtrContext : public antlr4::ParserRuleContext {
  public:
    PtrContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;

   
  };

  PtrContext* ptr();

  class  ArrayContext : public antlr4::ParserRuleContext {
  public:
    ArrayContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *Int();

   
  };

  ArrayContext* array();

  class  RequiresBlockContext : public antlr4::ParserRuleContext {
  public:
    RequiresBlockContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<ReqPredContext *> reqPred();
    ReqPredContext* reqPred(size_t i);

   
  };

  RequiresBlockContext* requiresBlock();

  class  ReqPredContext : public antlr4::ParserRuleContext {
  public:
    antlr4::Token *neg = nullptr;;
    antlr4::Token *implication = nullptr;;
    antlr4::Token *seq = nullptr;;
    antlr4::Token *alt = nullptr;;
    ReqPredContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    ReqPredLitContext *reqPredLit();
    ConstrContext *constr();
    std::vector<ReqPredContext *> reqPred();
    ReqPredContext* reqPred(size_t i);

   
  };

  ReqPredContext* reqPred();
  ReqPredContext* reqPred(int precedence);
  class  ReqPredLitContext : public antlr4::ParserRuleContext {
  public:
    ReqPredLitContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    PredContext *pred();

   
  };

  ReqPredLitContext* reqPredLit();

  class  PredContext : public antlr4::ParserRuleContext {
  public:
    antlr4::Token *name = nullptr;;
    CrySLParser::SuParListContext *paramList = nullptr;;
    PredContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *Ident();
    SuParListContext *suParList();

   
  };

  PredContext* pred();

  class  SuParListContext : public antlr4::ParserRuleContext {
  public:
    SuParListContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<SuParContext *> suPar();
    SuParContext* suPar(size_t i);

   
  };

  SuParListContext* suParList();

  class  SuParContext : public antlr4::ParserRuleContext {
  public:
    CrySLParser::ConsPredContext *value = nullptr;;
    antlr4::Token *thisptr = nullptr;;
    antlr4::Token *wildcard = nullptr;;
    SuParContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    ConsPredContext *consPred();

   
  };

  SuParContext* suPar();

  class  ConsPredContext : public antlr4::ParserRuleContext {
  public:
    ConsPredContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    LiteralExprContext *literalExpr();

   
  };

  ConsPredContext* consPred();

  class  LiteralExprContext : public antlr4::ParserRuleContext {
  public:
    LiteralExprContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    LiteralContext *literal();
    MemberAccessContext *memberAccess();
    PreDefinedPredicateContext *preDefinedPredicate();

   
  };

  LiteralExprContext* literalExpr();

  class  LiteralContext : public antlr4::ParserRuleContext {
  public:
    antlr4::Token *base = nullptr;;
    antlr4::Token *pow = nullptr;;
    antlr4::Token *exp = nullptr;;
    LiteralContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<antlr4::tree::TerminalNode *> Int();
    antlr4::tree::TerminalNode* Int(size_t i);
    antlr4::tree::TerminalNode *Bool();
    antlr4::tree::TerminalNode *String();

   
  };

  LiteralContext* literal();

  class  MemberAccessContext : public antlr4::ParserRuleContext {
  public:
    antlr4::Token *deref = nullptr;;
    antlr4::Token *dot = nullptr;;
    antlr4::Token *arrow = nullptr;;
    MemberAccessContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<antlr4::tree::TerminalNode *> Ident();
    antlr4::tree::TerminalNode* Ident(size_t i);

   
  };

  MemberAccessContext* memberAccess();

  class  PreDefinedPredicateContext : public antlr4::ParserRuleContext {
  public:
    antlr4::Token *name = nullptr;;
    CrySLParser::MemberAccessContext *obj = nullptr;;
    CrySLParser::TypeNameContext *type = nullptr;;
    antlr4::Token *evt = nullptr;;
    PreDefinedPredicateContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    MemberAccessContext *memberAccess();
    TypeNameContext *typeName();
    antlr4::tree::TerminalNode *Ident();

   
  };

  PreDefinedPredicateContext* preDefinedPredicate();

  class  EnsuresContext : public antlr4::ParserRuleContext {
  public:
    EnsuresContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<EnsPredContext *> ensPred();
    EnsPredContext* ensPred(size_t i);

   
  };

  EnsuresContext* ensures();

  class  EnsPredContext : public antlr4::ParserRuleContext {
  public:
    antlr4::Token *state = nullptr;;
    EnsPredContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    PredContext *pred();
    ConstrContext *constr();
    antlr4::tree::TerminalNode *Ident();

   
  };

  EnsPredContext* ensPred();

  class  ConstraintsContext : public antlr4::ParserRuleContext {
  public:
    ConstraintsContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<ConstrContext *> constr();
    ConstrContext* constr(size_t i);

   
  };

  ConstraintsContext* constraints();

  class  ConstrContext : public antlr4::ParserRuleContext {
  public:
    antlr4::Token *lnot = nullptr;;
    antlr4::Token *mul = nullptr;;
    antlr4::Token *div = nullptr;;
    antlr4::Token *mod = nullptr;;
    antlr4::Token *plus = nullptr;;
    antlr4::Token *minus = nullptr;;
    antlr4::Token *equal = nullptr;;
    antlr4::Token *unequal = nullptr;;
    antlr4::Token *land = nullptr;;
    antlr4::Token *lor = nullptr;;
    antlr4::Token *implies = nullptr;;
    ConstrContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<ConstrContext *> constr();
    ConstrContext* constr(size_t i);
    ConsContext *cons();
    ComparingRelOperatorContext *comparingRelOperator();

   
  };

  ConstrContext* constr();
  ConstrContext* constr(int precedence);
  class  ComparingRelOperatorContext : public antlr4::ParserRuleContext {
  public:
    antlr4::Token *less = nullptr;;
    antlr4::Token *less_or_equal = nullptr;;
    antlr4::Token *greater_or_equal = nullptr;;
    antlr4::Token *greater = nullptr;;
    ComparingRelOperatorContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;

   
  };

  ComparingRelOperatorContext* comparingRelOperator();

  class  ConsContext : public antlr4::ParserRuleContext {
  public:
    ConsContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    ArrayElementsContext *arrayElements();
    LitListContext *litList();
    LiteralExprContext *literalExpr();

   
  };

  ConsContext* cons();

  class  ArrayElementsContext : public antlr4::ParserRuleContext {
  public:
    antlr4::Token *el = nullptr;;
    ArrayElementsContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    ConsPredContext *consPred();

   
  };

  ArrayElementsContext* arrayElements();

  class  LitListContext : public antlr4::ParserRuleContext {
  public:
    antlr4::Token *ellipsis = nullptr;;
    LitListContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<LiteralContext *> literal();
    LiteralContext* literal(size_t i);

   
  };

  LitListContext* litList();

  class  EventsContext : public antlr4::ParserRuleContext {
  public:
    EventsContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<EventsOccurenceContext *> eventsOccurence();
    EventsOccurenceContext* eventsOccurence(size_t i);
    std::vector<EventAggregateContext *> eventAggregate();
    EventAggregateContext* eventAggregate(size_t i);

   
  };

  EventsContext* events();

  class  EventsOccurenceContext : public antlr4::ParserRuleContext {
  public:
    antlr4::Token *eventName = nullptr;;
    antlr4::Token *returnValue = nullptr;;
    antlr4::Token *returnThis = nullptr;;
    antlr4::Token *methodName = nullptr;;
    EventsOccurenceContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<antlr4::tree::TerminalNode *> Ident();
    antlr4::tree::TerminalNode* Ident(size_t i);
    ParametersListContext *parametersList();

   
  };

  EventsOccurenceContext* eventsOccurence();

  class  ParametersListContext : public antlr4::ParserRuleContext {
  public:
    ParametersListContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<ParamContext *> param();
    ParamContext* param(size_t i);

   
  };

  ParametersListContext* parametersList();

  class  ParamContext : public antlr4::ParserRuleContext {
  public:
    antlr4::Token *thisPtr = nullptr;;
    antlr4::Token *wildCard = nullptr;;
    ParamContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    MemberAccessContext *memberAccess();

   
  };

  ParamContext* param();

  class  EventAggregateContext : public antlr4::ParserRuleContext {
  public:
    antlr4::Token *eventName = nullptr;;
    EventAggregateContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    AggContext *agg();
    antlr4::tree::TerminalNode *Ident();

   
  };

  EventAggregateContext* eventAggregate();

  class  AggContext : public antlr4::ParserRuleContext {
  public:
    AggContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<antlr4::tree::TerminalNode *> Ident();
    antlr4::tree::TerminalNode* Ident(size_t i);

   
  };

  AggContext* agg();

  class  OrderContext : public antlr4::ParserRuleContext {
  public:
    OrderContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    OrderSequenceContext *orderSequence();

   
  };

  OrderContext* order();

  class  OrderSequenceContext : public antlr4::ParserRuleContext {
  public:
    OrderSequenceContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<SimpleOrderContext *> simpleOrder();
    SimpleOrderContext* simpleOrder(size_t i);

   
  };

  OrderSequenceContext* orderSequence();

  class  SimpleOrderContext : public antlr4::ParserRuleContext {
  public:
    SimpleOrderContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<UnorderedSymbolsContext *> unorderedSymbols();
    UnorderedSymbolsContext* unorderedSymbols(size_t i);

   
  };

  SimpleOrderContext* simpleOrder();

  class  UnorderedSymbolsContext : public antlr4::ParserRuleContext {
  public:
    antlr4::Token *lower = nullptr;;
    antlr4::Token *bound = nullptr;;
    antlr4::Token *upper = nullptr;;
    UnorderedSymbolsContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<PrimaryContext *> primary();
    PrimaryContext* primary(size_t i);
    std::vector<antlr4::tree::TerminalNode *> Int();
    antlr4::tree::TerminalNode* Int(size_t i);

   
  };

  UnorderedSymbolsContext* unorderedSymbols();

  class  PrimaryContext : public antlr4::ParserRuleContext {
  public:
    antlr4::Token *eventName = nullptr;;
    antlr4::Token *elementop = nullptr;;
    PrimaryContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *Ident();
    OrderSequenceContext *orderSequence();

   
  };

  PrimaryContext* primary();

  class  NegatesContext : public antlr4::ParserRuleContext {
  public:
    NegatesContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<NegatesOccurenceContext *> negatesOccurence();
    NegatesOccurenceContext* negatesOccurence(size_t i);

   
  };

  NegatesContext* negates();

  class  NegatesOccurenceContext : public antlr4::ParserRuleContext {
  public:
    NegatesOccurenceContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    EnsPredContext *ensPred();

   
  };

  NegatesOccurenceContext* negatesOccurence();

  class  ForbiddenContext : public antlr4::ParserRuleContext {
  public:
    ForbiddenContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<ForbiddenOccurenceContext *> forbiddenOccurence();
    ForbiddenOccurenceContext* forbiddenOccurence(size_t i);

   
  };

  ForbiddenContext* forbidden();

  class  ForbiddenOccurenceContext : public antlr4::ParserRuleContext {
  public:
    CrySLParser::FqnContext *methodName = nullptr;;
    antlr4::Token *eventName = nullptr;;
    ForbiddenOccurenceContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    FqnContext *fqn();
    antlr4::tree::TerminalNode *Ident();

   
  };

  ForbiddenOccurenceContext* forbiddenOccurence();

  class  FqnContext : public antlr4::ParserRuleContext {
  public:
    FqnContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    QualifiedNameContext *qualifiedName();
    TypeNameListContext *typeNameList();

   
  };

  FqnContext* fqn();

  class  TypeNameListContext : public antlr4::ParserRuleContext {
  public:
    TypeNameListContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<TypeNameContext *> typeName();
    TypeNameContext* typeName(size_t i);

   
  };

  TypeNameListContext* typeNameList();


  virtual bool sempred(antlr4::RuleContext *_localctx, size_t ruleIndex, size_t predicateIndex) override;
  bool reqPredSempred(ReqPredContext *_localctx, size_t predicateIndex);
  bool constrSempred(ConstrContext *_localctx, size_t predicateIndex);

private:
  static std::vector<antlr4::dfa::DFA> _decisionToDFA;
  static antlr4::atn::PredictionContextCache _sharedContextCache;
  static std::vector<std::string> _ruleNames;
  static std::vector<std::string> _tokenNames;

  static std::vector<std::string> _literalNames;
  static std::vector<std::string> _symbolicNames;
  static antlr4::dfa::Vocabulary _vocabulary;
  static antlr4::atn::ATN _atn;
  static std::vector<uint16_t> _serializedATN;


  struct Initializer {
    Initializer();
  };
  static Initializer _init;
};

