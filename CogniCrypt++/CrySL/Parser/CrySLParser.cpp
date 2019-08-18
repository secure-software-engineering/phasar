
// Generated from .\CrySL.g4 by ANTLR 4.7.2



#include "CrySLParser.h"


using namespace antlrcpp;
using namespace antlr4;

CrySLParser::CrySLParser(TokenStream *input) : Parser(input) {
  _interpreter = new atn::ParserATNSimulator(this, _atn, _decisionToDFA, _sharedContextCache);
}

CrySLParser::~CrySLParser() {
  delete _interpreter;
}

std::string CrySLParser::getGrammarFileName() const {
  return "CrySL.g4";
}

const std::vector<std::string>& CrySLParser::getRuleNames() const {
  return _ruleNames;
}

dfa::Vocabulary& CrySLParser::getVocabulary() const {
  return _vocabulary;
}


//----------------- DomainModelContext ------------------------------------------------------------------

CrySLParser::DomainModelContext::DomainModelContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

CrySLParser::SpecContext* CrySLParser::DomainModelContext::spec() {
  return getRuleContext<CrySLParser::SpecContext>(0);
}

CrySLParser::ObjectsContext* CrySLParser::DomainModelContext::objects() {
  return getRuleContext<CrySLParser::ObjectsContext>(0);
}

CrySLParser::EventsContext* CrySLParser::DomainModelContext::events() {
  return getRuleContext<CrySLParser::EventsContext>(0);
}

CrySLParser::OrderContext* CrySLParser::DomainModelContext::order() {
  return getRuleContext<CrySLParser::OrderContext>(0);
}

tree::TerminalNode* CrySLParser::DomainModelContext::EOF() {
  return getToken(CrySLParser::EOF, 0);
}

CrySLParser::ForbiddenContext* CrySLParser::DomainModelContext::forbidden() {
  return getRuleContext<CrySLParser::ForbiddenContext>(0);
}

CrySLParser::ConstraintsContext* CrySLParser::DomainModelContext::constraints() {
  return getRuleContext<CrySLParser::ConstraintsContext>(0);
}

CrySLParser::RequiresBlockContext* CrySLParser::DomainModelContext::requiresBlock() {
  return getRuleContext<CrySLParser::RequiresBlockContext>(0);
}

CrySLParser::EnsuresContext* CrySLParser::DomainModelContext::ensures() {
  return getRuleContext<CrySLParser::EnsuresContext>(0);
}

CrySLParser::NegatesContext* CrySLParser::DomainModelContext::negates() {
  return getRuleContext<CrySLParser::NegatesContext>(0);
}


size_t CrySLParser::DomainModelContext::getRuleIndex() const {
  return CrySLParser::RuleDomainModel;
}


CrySLParser::DomainModelContext* CrySLParser::domainModel() {
  DomainModelContext *_localctx = _tracker.createInstance<DomainModelContext>(_ctx, getState());
  enterRule(_localctx, 0, CrySLParser::RuleDomainModel);
  size_t _la = 0;

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(82);
    spec();
    setState(83);
    objects();
    setState(85);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == CrySLParser::T__51) {
      setState(84);
      forbidden();
    }
    setState(87);
    events();
    setState(88);
    order();
    setState(90);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == CrySLParser::T__24) {
      setState(89);
      constraints();
    }
    setState(93);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == CrySLParser::T__7) {
      setState(92);
      requiresBlock();
    }
    setState(96);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == CrySLParser::T__22) {
      setState(95);
      ensures();
    }
    setState(99);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == CrySLParser::T__50) {
      setState(98);
      negates();
    }
    setState(101);
    match(CrySLParser::EOF);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- SpecContext ------------------------------------------------------------------

CrySLParser::SpecContext::SpecContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

CrySLParser::QualifiedNameContext* CrySLParser::SpecContext::qualifiedName() {
  return getRuleContext<CrySLParser::QualifiedNameContext>(0);
}


size_t CrySLParser::SpecContext::getRuleIndex() const {
  return CrySLParser::RuleSpec;
}


CrySLParser::SpecContext* CrySLParser::spec() {
  SpecContext *_localctx = _tracker.createInstance<SpecContext>(_ctx, getState());
  enterRule(_localctx, 2, CrySLParser::RuleSpec);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(103);
    match(CrySLParser::T__0);
    setState(104);
    qualifiedName();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- QualifiedNameContext ------------------------------------------------------------------

CrySLParser::QualifiedNameContext::QualifiedNameContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<tree::TerminalNode *> CrySLParser::QualifiedNameContext::Ident() {
  return getTokens(CrySLParser::Ident);
}

tree::TerminalNode* CrySLParser::QualifiedNameContext::Ident(size_t i) {
  return getToken(CrySLParser::Ident, i);
}


size_t CrySLParser::QualifiedNameContext::getRuleIndex() const {
  return CrySLParser::RuleQualifiedName;
}


CrySLParser::QualifiedNameContext* CrySLParser::qualifiedName() {
  QualifiedNameContext *_localctx = _tracker.createInstance<QualifiedNameContext>(_ctx, getState());
  enterRule(_localctx, 4, CrySLParser::RuleQualifiedName);
  size_t _la = 0;

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(106);
    match(CrySLParser::Ident);
    setState(111);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == CrySLParser::T__1) {
      setState(107);
      match(CrySLParser::T__1);
      setState(108);
      match(CrySLParser::Ident);
      setState(113);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ObjectsContext ------------------------------------------------------------------

CrySLParser::ObjectsContext::ObjectsContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<CrySLParser::ObjectDeclContext *> CrySLParser::ObjectsContext::objectDecl() {
  return getRuleContexts<CrySLParser::ObjectDeclContext>();
}

CrySLParser::ObjectDeclContext* CrySLParser::ObjectsContext::objectDecl(size_t i) {
  return getRuleContext<CrySLParser::ObjectDeclContext>(i);
}


size_t CrySLParser::ObjectsContext::getRuleIndex() const {
  return CrySLParser::RuleObjects;
}


CrySLParser::ObjectsContext* CrySLParser::objects() {
  ObjectsContext *_localctx = _tracker.createInstance<ObjectsContext>(_ctx, getState());
  enterRule(_localctx, 6, CrySLParser::RuleObjects);
  size_t _la = 0;

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(114);
    match(CrySLParser::T__2);
    setState(118);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == CrySLParser::Ident) {
      setState(115);
      objectDecl();
      setState(120);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ObjectDeclContext ------------------------------------------------------------------

CrySLParser::ObjectDeclContext::ObjectDeclContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

CrySLParser::TypeNameContext* CrySLParser::ObjectDeclContext::typeName() {
  return getRuleContext<CrySLParser::TypeNameContext>(0);
}

tree::TerminalNode* CrySLParser::ObjectDeclContext::Ident() {
  return getToken(CrySLParser::Ident, 0);
}

std::vector<CrySLParser::ArrayContext *> CrySLParser::ObjectDeclContext::array() {
  return getRuleContexts<CrySLParser::ArrayContext>();
}

CrySLParser::ArrayContext* CrySLParser::ObjectDeclContext::array(size_t i) {
  return getRuleContext<CrySLParser::ArrayContext>(i);
}


size_t CrySLParser::ObjectDeclContext::getRuleIndex() const {
  return CrySLParser::RuleObjectDecl;
}


CrySLParser::ObjectDeclContext* CrySLParser::objectDecl() {
  ObjectDeclContext *_localctx = _tracker.createInstance<ObjectDeclContext>(_ctx, getState());
  enterRule(_localctx, 8, CrySLParser::RuleObjectDecl);
  size_t _la = 0;

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(121);
    typeName();
    setState(122);
    match(CrySLParser::Ident);
    setState(126);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == CrySLParser::T__5) {
      setState(123);
      array();
      setState(128);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(129);
    match(CrySLParser::T__3);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- TypeNameContext ------------------------------------------------------------------

CrySLParser::TypeNameContext::TypeNameContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

CrySLParser::QualifiedNameContext* CrySLParser::TypeNameContext::qualifiedName() {
  return getRuleContext<CrySLParser::QualifiedNameContext>(0);
}


size_t CrySLParser::TypeNameContext::getRuleIndex() const {
  return CrySLParser::RuleTypeName;
}


CrySLParser::TypeNameContext* CrySLParser::typeName() {
  TypeNameContext *_localctx = _tracker.createInstance<TypeNameContext>(_ctx, getState());
  enterRule(_localctx, 10, CrySLParser::RuleTypeName);
  size_t _la = 0;

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(131);
    qualifiedName();
    setState(135);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == CrySLParser::T__4) {
      setState(132);
      dynamic_cast<TypeNameContext *>(_localctx)->pointer = match(CrySLParser::T__4);
      setState(137);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ArrayContext ------------------------------------------------------------------

CrySLParser::ArrayContext::ArrayContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* CrySLParser::ArrayContext::Int() {
  return getToken(CrySLParser::Int, 0);
}


size_t CrySLParser::ArrayContext::getRuleIndex() const {
  return CrySLParser::RuleArray;
}


CrySLParser::ArrayContext* CrySLParser::array() {
  ArrayContext *_localctx = _tracker.createInstance<ArrayContext>(_ctx, getState());
  enterRule(_localctx, 12, CrySLParser::RuleArray);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(138);
    match(CrySLParser::T__5);
    setState(139);
    match(CrySLParser::Int);
    setState(140);
    match(CrySLParser::T__6);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- RequiresBlockContext ------------------------------------------------------------------

CrySLParser::RequiresBlockContext::RequiresBlockContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<CrySLParser::ReqPredContext *> CrySLParser::RequiresBlockContext::reqPred() {
  return getRuleContexts<CrySLParser::ReqPredContext>();
}

CrySLParser::ReqPredContext* CrySLParser::RequiresBlockContext::reqPred(size_t i) {
  return getRuleContext<CrySLParser::ReqPredContext>(i);
}


size_t CrySLParser::RequiresBlockContext::getRuleIndex() const {
  return CrySLParser::RuleRequiresBlock;
}


CrySLParser::RequiresBlockContext* CrySLParser::requiresBlock() {
  RequiresBlockContext *_localctx = _tracker.createInstance<RequiresBlockContext>(_ctx, getState());
  enterRule(_localctx, 14, CrySLParser::RuleRequiresBlock);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    size_t alt;
    enterOuterAlt(_localctx, 1);
    setState(142);
    match(CrySLParser::T__7);
    setState(148);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 9, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        setState(143);
        reqPred(0);
        setState(144);
        match(CrySLParser::T__3); 
      }
      setState(150);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 9, _ctx);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ReqPredContext ------------------------------------------------------------------

CrySLParser::ReqPredContext::ReqPredContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

CrySLParser::ReqPredLitContext* CrySLParser::ReqPredContext::reqPredLit() {
  return getRuleContext<CrySLParser::ReqPredLitContext>(0);
}

CrySLParser::ConstrContext* CrySLParser::ReqPredContext::constr() {
  return getRuleContext<CrySLParser::ConstrContext>(0);
}

std::vector<CrySLParser::ReqPredContext *> CrySLParser::ReqPredContext::reqPred() {
  return getRuleContexts<CrySLParser::ReqPredContext>();
}

CrySLParser::ReqPredContext* CrySLParser::ReqPredContext::reqPred(size_t i) {
  return getRuleContext<CrySLParser::ReqPredContext>(i);
}


size_t CrySLParser::ReqPredContext::getRuleIndex() const {
  return CrySLParser::RuleReqPred;
}



CrySLParser::ReqPredContext* CrySLParser::reqPred() {
   return reqPred(0);
}

CrySLParser::ReqPredContext* CrySLParser::reqPred(int precedence) {
  ParserRuleContext *parentContext = _ctx;
  size_t parentState = getState();
  CrySLParser::ReqPredContext *_localctx = _tracker.createInstance<ReqPredContext>(_ctx, parentState);
  CrySLParser::ReqPredContext *previousContext = _localctx;
  (void)previousContext; // Silence compiler, in case the context is not used by generated code.
  size_t startState = 16;
  enterRecursionRule(_localctx, 16, CrySLParser::RuleReqPred, precedence);

    size_t _la = 0;

  auto onExit = finally([=] {
    unrollRecursionContexts(parentContext);
  });
  try {
    size_t alt;
    enterOuterAlt(_localctx, 1);
    setState(160);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 11, _ctx)) {
    case 1: {
      setState(153);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == CrySLParser::T__8) {
        setState(152);
        dynamic_cast<ReqPredContext *>(_localctx)->neg = match(CrySLParser::T__8);
      }
      setState(155);
      reqPredLit();
      break;
    }

    case 2: {
      setState(156);
      constr(0);
      setState(157);
      dynamic_cast<ReqPredContext *>(_localctx)->implication = match(CrySLParser::T__11);
      setState(158);
      reqPred(1);
      break;
    }

    }
    _ctx->stop = _input->LT(-1);
    setState(170);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 13, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        if (!_parseListeners.empty())
          triggerExitRuleEvent();
        previousContext = _localctx;
        setState(168);
        _errHandler->sync(this);
        switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 12, _ctx)) {
        case 1: {
          _localctx = _tracker.createInstance<ReqPredContext>(parentContext, parentState);
          pushNewRecursionContext(_localctx, startState, RuleReqPred);
          setState(162);

          if (!(precpred(_ctx, 3))) throw FailedPredicateException(this, "precpred(_ctx, 3)");
          setState(163);
          dynamic_cast<ReqPredContext *>(_localctx)->seq = match(CrySLParser::T__9);
          setState(164);
          reqPred(4);
          break;
        }

        case 2: {
          _localctx = _tracker.createInstance<ReqPredContext>(parentContext, parentState);
          pushNewRecursionContext(_localctx, startState, RuleReqPred);
          setState(165);

          if (!(precpred(_ctx, 2))) throw FailedPredicateException(this, "precpred(_ctx, 2)");
          setState(166);
          dynamic_cast<ReqPredContext *>(_localctx)->alt = match(CrySLParser::T__10);
          setState(167);
          reqPred(3);
          break;
        }

        } 
      }
      setState(172);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 13, _ctx);
    }
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }
  return _localctx;
}

//----------------- ReqPredLitContext ------------------------------------------------------------------

CrySLParser::ReqPredLitContext::ReqPredLitContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

CrySLParser::PredContext* CrySLParser::ReqPredLitContext::pred() {
  return getRuleContext<CrySLParser::PredContext>(0);
}


size_t CrySLParser::ReqPredLitContext::getRuleIndex() const {
  return CrySLParser::RuleReqPredLit;
}


CrySLParser::ReqPredLitContext* CrySLParser::reqPredLit() {
  ReqPredLitContext *_localctx = _tracker.createInstance<ReqPredLitContext>(_ctx, getState());
  enterRule(_localctx, 18, CrySLParser::RuleReqPredLit);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(173);
    pred();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- PredContext ------------------------------------------------------------------

CrySLParser::PredContext::PredContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* CrySLParser::PredContext::Ident() {
  return getToken(CrySLParser::Ident, 0);
}

CrySLParser::SuParListContext* CrySLParser::PredContext::suParList() {
  return getRuleContext<CrySLParser::SuParListContext>(0);
}


size_t CrySLParser::PredContext::getRuleIndex() const {
  return CrySLParser::RulePred;
}


CrySLParser::PredContext* CrySLParser::pred() {
  PredContext *_localctx = _tracker.createInstance<PredContext>(_ctx, getState());
  enterRule(_localctx, 20, CrySLParser::RulePred);
  size_t _la = 0;

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(175);
    dynamic_cast<PredContext *>(_localctx)->name = match(CrySLParser::Ident);
    setState(176);
    match(CrySLParser::T__5);
    setState(178);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & ((1ULL << CrySLParser::T__4)
      | (1ULL << CrySLParser::T__12)
      | (1ULL << CrySLParser::T__13)
      | (1ULL << CrySLParser::T__17)
      | (1ULL << CrySLParser::T__18)
      | (1ULL << CrySLParser::T__19)
      | (1ULL << CrySLParser::T__20)
      | (1ULL << CrySLParser::T__21)
      | (1ULL << CrySLParser::Int)
      | (1ULL << CrySLParser::Bool)
      | (1ULL << CrySLParser::String)
      | (1ULL << CrySLParser::Ident))) != 0)) {
      setState(177);
      dynamic_cast<PredContext *>(_localctx)->paramList = suParList();
    }
    setState(180);
    match(CrySLParser::T__6);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- SuParListContext ------------------------------------------------------------------

CrySLParser::SuParListContext::SuParListContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<CrySLParser::SuParContext *> CrySLParser::SuParListContext::suPar() {
  return getRuleContexts<CrySLParser::SuParContext>();
}

CrySLParser::SuParContext* CrySLParser::SuParListContext::suPar(size_t i) {
  return getRuleContext<CrySLParser::SuParContext>(i);
}


size_t CrySLParser::SuParListContext::getRuleIndex() const {
  return CrySLParser::RuleSuParList;
}


CrySLParser::SuParListContext* CrySLParser::suParList() {
  SuParListContext *_localctx = _tracker.createInstance<SuParListContext>(_ctx, getState());
  enterRule(_localctx, 22, CrySLParser::RuleSuParList);
  size_t _la = 0;

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(182);
    suPar();
    setState(187);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == CrySLParser::T__9) {
      setState(183);
      match(CrySLParser::T__9);
      setState(184);
      suPar();
      setState(189);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- SuParContext ------------------------------------------------------------------

CrySLParser::SuParContext::SuParContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

CrySLParser::ConsPredContext* CrySLParser::SuParContext::consPred() {
  return getRuleContext<CrySLParser::ConsPredContext>(0);
}


size_t CrySLParser::SuParContext::getRuleIndex() const {
  return CrySLParser::RuleSuPar;
}


CrySLParser::SuParContext* CrySLParser::suPar() {
  SuParContext *_localctx = _tracker.createInstance<SuParContext>(_ctx, getState());
  enterRule(_localctx, 24, CrySLParser::RuleSuPar);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    setState(193);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case CrySLParser::T__4:
      case CrySLParser::T__17:
      case CrySLParser::T__18:
      case CrySLParser::T__19:
      case CrySLParser::T__20:
      case CrySLParser::T__21:
      case CrySLParser::Int:
      case CrySLParser::Bool:
      case CrySLParser::String:
      case CrySLParser::Ident: {
        enterOuterAlt(_localctx, 1);
        setState(190);
        dynamic_cast<SuParContext *>(_localctx)->value = consPred();
        break;
      }

      case CrySLParser::T__12: {
        enterOuterAlt(_localctx, 2);
        setState(191);
        dynamic_cast<SuParContext *>(_localctx)->thisptr = match(CrySLParser::T__12);
        break;
      }

      case CrySLParser::T__13: {
        enterOuterAlt(_localctx, 3);
        setState(192);
        dynamic_cast<SuParContext *>(_localctx)->wildcard = match(CrySLParser::T__13);
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ConsPredContext ------------------------------------------------------------------

CrySLParser::ConsPredContext::ConsPredContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

CrySLParser::LiteralExprContext* CrySLParser::ConsPredContext::literalExpr() {
  return getRuleContext<CrySLParser::LiteralExprContext>(0);
}


size_t CrySLParser::ConsPredContext::getRuleIndex() const {
  return CrySLParser::RuleConsPred;
}


CrySLParser::ConsPredContext* CrySLParser::consPred() {
  ConsPredContext *_localctx = _tracker.createInstance<ConsPredContext>(_ctx, getState());
  enterRule(_localctx, 26, CrySLParser::RuleConsPred);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(195);
    literalExpr();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- LiteralExprContext ------------------------------------------------------------------

CrySLParser::LiteralExprContext::LiteralExprContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

CrySLParser::LiteralContext* CrySLParser::LiteralExprContext::literal() {
  return getRuleContext<CrySLParser::LiteralContext>(0);
}

CrySLParser::MemberAccessContext* CrySLParser::LiteralExprContext::memberAccess() {
  return getRuleContext<CrySLParser::MemberAccessContext>(0);
}

CrySLParser::PreDefinedPredicateContext* CrySLParser::LiteralExprContext::preDefinedPredicate() {
  return getRuleContext<CrySLParser::PreDefinedPredicateContext>(0);
}


size_t CrySLParser::LiteralExprContext::getRuleIndex() const {
  return CrySLParser::RuleLiteralExpr;
}


CrySLParser::LiteralExprContext* CrySLParser::literalExpr() {
  LiteralExprContext *_localctx = _tracker.createInstance<LiteralExprContext>(_ctx, getState());
  enterRule(_localctx, 28, CrySLParser::RuleLiteralExpr);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    setState(200);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case CrySLParser::Int:
      case CrySLParser::Bool:
      case CrySLParser::String: {
        enterOuterAlt(_localctx, 1);
        setState(197);
        literal();
        break;
      }

      case CrySLParser::T__4:
      case CrySLParser::Ident: {
        enterOuterAlt(_localctx, 2);
        setState(198);
        memberAccess();
        break;
      }

      case CrySLParser::T__17:
      case CrySLParser::T__18:
      case CrySLParser::T__19:
      case CrySLParser::T__20:
      case CrySLParser::T__21: {
        enterOuterAlt(_localctx, 3);
        setState(199);
        preDefinedPredicate();
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- LiteralContext ------------------------------------------------------------------

CrySLParser::LiteralContext::LiteralContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<tree::TerminalNode *> CrySLParser::LiteralContext::Int() {
  return getTokens(CrySLParser::Int);
}

tree::TerminalNode* CrySLParser::LiteralContext::Int(size_t i) {
  return getToken(CrySLParser::Int, i);
}

tree::TerminalNode* CrySLParser::LiteralContext::Bool() {
  return getToken(CrySLParser::Bool, 0);
}

tree::TerminalNode* CrySLParser::LiteralContext::String() {
  return getToken(CrySLParser::String, 0);
}


size_t CrySLParser::LiteralContext::getRuleIndex() const {
  return CrySLParser::RuleLiteral;
}


CrySLParser::LiteralContext* CrySLParser::literal() {
  LiteralContext *_localctx = _tracker.createInstance<LiteralContext>(_ctx, getState());
  enterRule(_localctx, 30, CrySLParser::RuleLiteral);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    setState(208);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 18, _ctx)) {
    case 1: {
      enterOuterAlt(_localctx, 1);
      setState(202);
      match(CrySLParser::Int);
      break;
    }

    case 2: {
      enterOuterAlt(_localctx, 2);
      setState(203);
      dynamic_cast<LiteralContext *>(_localctx)->base = match(CrySLParser::Int);
      setState(204);
      dynamic_cast<LiteralContext *>(_localctx)->pow = match(CrySLParser::T__14);
      setState(205);
      dynamic_cast<LiteralContext *>(_localctx)->exp = match(CrySLParser::Int);
      break;
    }

    case 3: {
      enterOuterAlt(_localctx, 3);
      setState(206);
      match(CrySLParser::Bool);
      break;
    }

    case 4: {
      enterOuterAlt(_localctx, 4);
      setState(207);
      match(CrySLParser::String);
      break;
    }

    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- MemberAccessContext ------------------------------------------------------------------

CrySLParser::MemberAccessContext::MemberAccessContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<tree::TerminalNode *> CrySLParser::MemberAccessContext::Ident() {
  return getTokens(CrySLParser::Ident);
}

tree::TerminalNode* CrySLParser::MemberAccessContext::Ident(size_t i) {
  return getToken(CrySLParser::Ident, i);
}


size_t CrySLParser::MemberAccessContext::getRuleIndex() const {
  return CrySLParser::RuleMemberAccess;
}


CrySLParser::MemberAccessContext* CrySLParser::memberAccess() {
  MemberAccessContext *_localctx = _tracker.createInstance<MemberAccessContext>(_ctx, getState());
  enterRule(_localctx, 32, CrySLParser::RuleMemberAccess);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    setState(219);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 19, _ctx)) {
    case 1: {
      enterOuterAlt(_localctx, 1);
      setState(210);
      match(CrySLParser::Ident);
      break;
    }

    case 2: {
      enterOuterAlt(_localctx, 2);
      setState(211);
      dynamic_cast<MemberAccessContext *>(_localctx)->deref = match(CrySLParser::T__4);
      setState(212);
      match(CrySLParser::Ident);
      break;
    }

    case 3: {
      enterOuterAlt(_localctx, 3);
      setState(213);
      match(CrySLParser::Ident);
      setState(214);
      dynamic_cast<MemberAccessContext *>(_localctx)->dot = match(CrySLParser::T__15);
      setState(215);
      match(CrySLParser::Ident);
      break;
    }

    case 4: {
      enterOuterAlt(_localctx, 4);
      setState(216);
      match(CrySLParser::Ident);
      setState(217);
      dynamic_cast<MemberAccessContext *>(_localctx)->arrow = match(CrySLParser::T__16);
      setState(218);
      match(CrySLParser::Ident);
      break;
    }

    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- PreDefinedPredicateContext ------------------------------------------------------------------

CrySLParser::PreDefinedPredicateContext::PreDefinedPredicateContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

CrySLParser::MemberAccessContext* CrySLParser::PreDefinedPredicateContext::memberAccess() {
  return getRuleContext<CrySLParser::MemberAccessContext>(0);
}

CrySLParser::TypeNameContext* CrySLParser::PreDefinedPredicateContext::typeName() {
  return getRuleContext<CrySLParser::TypeNameContext>(0);
}

tree::TerminalNode* CrySLParser::PreDefinedPredicateContext::Ident() {
  return getToken(CrySLParser::Ident, 0);
}


size_t CrySLParser::PreDefinedPredicateContext::getRuleIndex() const {
  return CrySLParser::RulePreDefinedPredicate;
}


CrySLParser::PreDefinedPredicateContext* CrySLParser::preDefinedPredicate() {
  PreDefinedPredicateContext *_localctx = _tracker.createInstance<PreDefinedPredicateContext>(_ctx, getState());
  enterRule(_localctx, 34, CrySLParser::RulePreDefinedPredicate);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    setState(246);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case CrySLParser::T__17: {
        enterOuterAlt(_localctx, 1);
        setState(221);
        dynamic_cast<PreDefinedPredicateContext *>(_localctx)->name = match(CrySLParser::T__17);
        setState(222);
        match(CrySLParser::T__5);
        setState(223);
        dynamic_cast<PreDefinedPredicateContext *>(_localctx)->obj = memberAccess();
        setState(224);
        match(CrySLParser::T__9);
        setState(225);
        dynamic_cast<PreDefinedPredicateContext *>(_localctx)->type = typeName();
        setState(226);
        match(CrySLParser::T__6);
        break;
      }

      case CrySLParser::T__18: {
        enterOuterAlt(_localctx, 2);
        setState(228);
        dynamic_cast<PreDefinedPredicateContext *>(_localctx)->name = match(CrySLParser::T__18);
        setState(229);
        match(CrySLParser::T__5);
        setState(230);
        dynamic_cast<PreDefinedPredicateContext *>(_localctx)->evt = match(CrySLParser::Ident);
        setState(231);
        match(CrySLParser::T__6);
        break;
      }

      case CrySLParser::T__19: {
        enterOuterAlt(_localctx, 3);
        setState(232);
        dynamic_cast<PreDefinedPredicateContext *>(_localctx)->name = match(CrySLParser::T__19);
        setState(233);
        match(CrySLParser::T__5);
        setState(234);
        dynamic_cast<PreDefinedPredicateContext *>(_localctx)->evt = match(CrySLParser::Ident);
        setState(235);
        match(CrySLParser::T__6);
        break;
      }

      case CrySLParser::T__20: {
        enterOuterAlt(_localctx, 4);
        setState(236);
        dynamic_cast<PreDefinedPredicateContext *>(_localctx)->name = match(CrySLParser::T__20);
        setState(237);
        match(CrySLParser::T__5);
        setState(238);
        dynamic_cast<PreDefinedPredicateContext *>(_localctx)->obj = memberAccess();
        setState(239);
        match(CrySLParser::T__6);
        break;
      }

      case CrySLParser::T__21: {
        enterOuterAlt(_localctx, 5);
        setState(241);
        dynamic_cast<PreDefinedPredicateContext *>(_localctx)->name = match(CrySLParser::T__21);
        setState(242);
        match(CrySLParser::T__5);
        setState(243);
        dynamic_cast<PreDefinedPredicateContext *>(_localctx)->obj = memberAccess();
        setState(244);
        match(CrySLParser::T__6);
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- EnsuresContext ------------------------------------------------------------------

CrySLParser::EnsuresContext::EnsuresContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<CrySLParser::EnsPredContext *> CrySLParser::EnsuresContext::ensPred() {
  return getRuleContexts<CrySLParser::EnsPredContext>();
}

CrySLParser::EnsPredContext* CrySLParser::EnsuresContext::ensPred(size_t i) {
  return getRuleContext<CrySLParser::EnsPredContext>(i);
}


size_t CrySLParser::EnsuresContext::getRuleIndex() const {
  return CrySLParser::RuleEnsures;
}


CrySLParser::EnsuresContext* CrySLParser::ensures() {
  EnsuresContext *_localctx = _tracker.createInstance<EnsuresContext>(_ctx, getState());
  enterRule(_localctx, 36, CrySLParser::RuleEnsures);
  size_t _la = 0;

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(248);
    match(CrySLParser::T__22);
    setState(252); 
    _errHandler->sync(this);
    _la = _input->LA(1);
    do {
      setState(249);
      ensPred();
      setState(250);
      match(CrySLParser::T__3);
      setState(254); 
      _errHandler->sync(this);
      _la = _input->LA(1);
    } while (_la == CrySLParser::Ident);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- EnsPredContext ------------------------------------------------------------------

CrySLParser::EnsPredContext::EnsPredContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

CrySLParser::PredContext* CrySLParser::EnsPredContext::pred() {
  return getRuleContext<CrySLParser::PredContext>(0);
}

tree::TerminalNode* CrySLParser::EnsPredContext::Ident() {
  return getToken(CrySLParser::Ident, 0);
}


size_t CrySLParser::EnsPredContext::getRuleIndex() const {
  return CrySLParser::RuleEnsPred;
}


CrySLParser::EnsPredContext* CrySLParser::ensPred() {
  EnsPredContext *_localctx = _tracker.createInstance<EnsPredContext>(_ctx, getState());
  enterRule(_localctx, 38, CrySLParser::RuleEnsPred);
  size_t _la = 0;

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(256);
    pred();
    setState(259);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == CrySLParser::T__23) {
      setState(257);
      match(CrySLParser::T__23);
      setState(258);
      dynamic_cast<EnsPredContext *>(_localctx)->state = match(CrySLParser::Ident);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ConstraintsContext ------------------------------------------------------------------

CrySLParser::ConstraintsContext::ConstraintsContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<CrySLParser::ConstrContext *> CrySLParser::ConstraintsContext::constr() {
  return getRuleContexts<CrySLParser::ConstrContext>();
}

CrySLParser::ConstrContext* CrySLParser::ConstraintsContext::constr(size_t i) {
  return getRuleContext<CrySLParser::ConstrContext>(i);
}


size_t CrySLParser::ConstraintsContext::getRuleIndex() const {
  return CrySLParser::RuleConstraints;
}


CrySLParser::ConstraintsContext* CrySLParser::constraints() {
  ConstraintsContext *_localctx = _tracker.createInstance<ConstraintsContext>(_ctx, getState());
  enterRule(_localctx, 40, CrySLParser::RuleConstraints);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    size_t alt;
    enterOuterAlt(_localctx, 1);
    setState(261);
    match(CrySLParser::T__24);
    setState(265); 
    _errHandler->sync(this);
    alt = 1;
    do {
      switch (alt) {
        case 1: {
              setState(262);
              constr(0);
              setState(263);
              match(CrySLParser::T__3);
              break;
            }

      default:
        throw NoViableAltException(this);
      }
      setState(267); 
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 23, _ctx);
    } while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ConstrContext ------------------------------------------------------------------

CrySLParser::ConstrContext::ConstrContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<CrySLParser::ConstrContext *> CrySLParser::ConstrContext::constr() {
  return getRuleContexts<CrySLParser::ConstrContext>();
}

CrySLParser::ConstrContext* CrySLParser::ConstrContext::constr(size_t i) {
  return getRuleContext<CrySLParser::ConstrContext>(i);
}

CrySLParser::ConsContext* CrySLParser::ConstrContext::cons() {
  return getRuleContext<CrySLParser::ConsContext>(0);
}

CrySLParser::ComparingRelOperatorContext* CrySLParser::ConstrContext::comparingRelOperator() {
  return getRuleContext<CrySLParser::ComparingRelOperatorContext>(0);
}


size_t CrySLParser::ConstrContext::getRuleIndex() const {
  return CrySLParser::RuleConstr;
}



CrySLParser::ConstrContext* CrySLParser::constr() {
   return constr(0);
}

CrySLParser::ConstrContext* CrySLParser::constr(int precedence) {
  ParserRuleContext *parentContext = _ctx;
  size_t parentState = getState();
  CrySLParser::ConstrContext *_localctx = _tracker.createInstance<ConstrContext>(_ctx, parentState);
  CrySLParser::ConstrContext *previousContext = _localctx;
  (void)previousContext; // Silence compiler, in case the context is not used by generated code.
  size_t startState = 42;
  enterRecursionRule(_localctx, 42, CrySLParser::RuleConstr, precedence);

    

  auto onExit = finally([=] {
    unrollRecursionContexts(parentContext);
  });
  try {
    size_t alt;
    enterOuterAlt(_localctx, 1);
    setState(277);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 24, _ctx)) {
    case 1: {
      break;
    }

    case 2: {
      setState(270);
      match(CrySLParser::T__25);
      setState(271);
      constr(0);
      setState(272);
      match(CrySLParser::T__26);
      break;
    }

    case 3: {
      setState(274);
      cons();
      break;
    }

    case 4: {
      setState(275);
      dynamic_cast<ConstrContext *>(_localctx)->lnot = match(CrySLParser::T__8);
      setState(276);
      constr(9);
      break;
    }

    }
    _ctx->stop = _input->LT(-1);
    setState(315);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 29, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        if (!_parseListeners.empty())
          triggerExitRuleEvent();
        previousContext = _localctx;
        setState(313);
        _errHandler->sync(this);
        switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 28, _ctx)) {
        case 1: {
          _localctx = _tracker.createInstance<ConstrContext>(parentContext, parentState);
          pushNewRecursionContext(_localctx, startState, RuleConstr);
          setState(279);

          if (!(precpred(_ctx, 8))) throw FailedPredicateException(this, "precpred(_ctx, 8)");
          setState(282);
          _errHandler->sync(this);
          switch (_input->LA(1)) {
            case CrySLParser::T__4: {
              setState(280);
              dynamic_cast<ConstrContext *>(_localctx)->mul = match(CrySLParser::T__4);
              break;
            }

            case CrySLParser::T__27: {
              setState(281);
              dynamic_cast<ConstrContext *>(_localctx)->div = match(CrySLParser::T__27);
              break;
            }

          default:
            throw NoViableAltException(this);
          }
          setState(284);
          constr(9);
          break;
        }

        case 2: {
          _localctx = _tracker.createInstance<ConstrContext>(parentContext, parentState);
          pushNewRecursionContext(_localctx, startState, RuleConstr);
          setState(285);

          if (!(precpred(_ctx, 7))) throw FailedPredicateException(this, "precpred(_ctx, 7)");
          setState(286);
          dynamic_cast<ConstrContext *>(_localctx)->mod = match(CrySLParser::T__28);
          setState(287);
          constr(8);
          break;
        }

        case 3: {
          _localctx = _tracker.createInstance<ConstrContext>(parentContext, parentState);
          pushNewRecursionContext(_localctx, startState, RuleConstr);
          setState(288);

          if (!(precpred(_ctx, 6))) throw FailedPredicateException(this, "precpred(_ctx, 6)");
          setState(291);
          _errHandler->sync(this);
          switch (_input->LA(1)) {
            case CrySLParser::T__29: {
              setState(289);
              dynamic_cast<ConstrContext *>(_localctx)->plus = match(CrySLParser::T__29);
              break;
            }

            case CrySLParser::T__30: {
              setState(290);
              dynamic_cast<ConstrContext *>(_localctx)->minus = match(CrySLParser::T__30);
              break;
            }

          default:
            throw NoViableAltException(this);
          }
          setState(293);
          constr(7);
          break;
        }

        case 4: {
          _localctx = _tracker.createInstance<ConstrContext>(parentContext, parentState);
          pushNewRecursionContext(_localctx, startState, RuleConstr);
          setState(294);

          if (!(precpred(_ctx, 5))) throw FailedPredicateException(this, "precpred(_ctx, 5)");
          setState(295);
          comparingRelOperator();
          setState(296);
          constr(6);
          break;
        }

        case 5: {
          _localctx = _tracker.createInstance<ConstrContext>(parentContext, parentState);
          pushNewRecursionContext(_localctx, startState, RuleConstr);
          setState(298);

          if (!(precpred(_ctx, 4))) throw FailedPredicateException(this, "precpred(_ctx, 4)");
          setState(301);
          _errHandler->sync(this);
          switch (_input->LA(1)) {
            case CrySLParser::T__31: {
              setState(299);
              dynamic_cast<ConstrContext *>(_localctx)->equal = match(CrySLParser::T__31);
              break;
            }

            case CrySLParser::T__32: {
              setState(300);
              dynamic_cast<ConstrContext *>(_localctx)->unequal = match(CrySLParser::T__32);
              break;
            }

          default:
            throw NoViableAltException(this);
          }
          setState(303);
          constr(5);
          break;
        }

        case 6: {
          _localctx = _tracker.createInstance<ConstrContext>(parentContext, parentState);
          pushNewRecursionContext(_localctx, startState, RuleConstr);
          setState(304);

          if (!(precpred(_ctx, 3))) throw FailedPredicateException(this, "precpred(_ctx, 3)");
          setState(305);
          dynamic_cast<ConstrContext *>(_localctx)->land = match(CrySLParser::T__33);
          setState(306);
          constr(4);
          break;
        }

        case 7: {
          _localctx = _tracker.createInstance<ConstrContext>(parentContext, parentState);
          pushNewRecursionContext(_localctx, startState, RuleConstr);
          setState(307);

          if (!(precpred(_ctx, 2))) throw FailedPredicateException(this, "precpred(_ctx, 2)");
          setState(308);
          dynamic_cast<ConstrContext *>(_localctx)->lor = match(CrySLParser::T__10);
          setState(309);
          constr(3);
          break;
        }

        case 8: {
          _localctx = _tracker.createInstance<ConstrContext>(parentContext, parentState);
          pushNewRecursionContext(_localctx, startState, RuleConstr);
          setState(310);

          if (!(precpred(_ctx, 1))) throw FailedPredicateException(this, "precpred(_ctx, 1)");
          setState(311);
          dynamic_cast<ConstrContext *>(_localctx)->implies = match(CrySLParser::T__11);
          setState(312);
          constr(1);
          break;
        }

        } 
      }
      setState(317);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 29, _ctx);
    }
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }
  return _localctx;
}

//----------------- ComparingRelOperatorContext ------------------------------------------------------------------

CrySLParser::ComparingRelOperatorContext::ComparingRelOperatorContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t CrySLParser::ComparingRelOperatorContext::getRuleIndex() const {
  return CrySLParser::RuleComparingRelOperator;
}


CrySLParser::ComparingRelOperatorContext* CrySLParser::comparingRelOperator() {
  ComparingRelOperatorContext *_localctx = _tracker.createInstance<ComparingRelOperatorContext>(_ctx, getState());
  enterRule(_localctx, 44, CrySLParser::RuleComparingRelOperator);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    setState(322);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case CrySLParser::T__34: {
        enterOuterAlt(_localctx, 1);
        setState(318);
        dynamic_cast<ComparingRelOperatorContext *>(_localctx)->less = match(CrySLParser::T__34);
        break;
      }

      case CrySLParser::T__35: {
        enterOuterAlt(_localctx, 2);
        setState(319);
        dynamic_cast<ComparingRelOperatorContext *>(_localctx)->less_or_equal = match(CrySLParser::T__35);
        break;
      }

      case CrySLParser::T__36: {
        enterOuterAlt(_localctx, 3);
        setState(320);
        dynamic_cast<ComparingRelOperatorContext *>(_localctx)->greater_or_equal = match(CrySLParser::T__36);
        break;
      }

      case CrySLParser::T__37: {
        enterOuterAlt(_localctx, 4);
        setState(321);
        dynamic_cast<ComparingRelOperatorContext *>(_localctx)->greater = match(CrySLParser::T__37);
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ConsContext ------------------------------------------------------------------

CrySLParser::ConsContext::ConsContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

CrySLParser::ArrayElementsContext* CrySLParser::ConsContext::arrayElements() {
  return getRuleContext<CrySLParser::ArrayElementsContext>(0);
}

CrySLParser::LitListContext* CrySLParser::ConsContext::litList() {
  return getRuleContext<CrySLParser::LitListContext>(0);
}

CrySLParser::LiteralExprContext* CrySLParser::ConsContext::literalExpr() {
  return getRuleContext<CrySLParser::LiteralExprContext>(0);
}


size_t CrySLParser::ConsContext::getRuleIndex() const {
  return CrySLParser::RuleCons;
}


CrySLParser::ConsContext* CrySLParser::cons() {
  ConsContext *_localctx = _tracker.createInstance<ConsContext>(_ctx, getState());
  enterRule(_localctx, 46, CrySLParser::RuleCons);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    setState(331);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 31, _ctx)) {
    case 1: {
      enterOuterAlt(_localctx, 1);
      setState(324);
      arrayElements();
      setState(325);
      match(CrySLParser::T__38);
      setState(326);
      match(CrySLParser::T__39);
      setState(327);
      litList();
      setState(328);
      match(CrySLParser::T__40);
      break;
    }

    case 2: {
      enterOuterAlt(_localctx, 2);
      setState(330);
      literalExpr();
      break;
    }

    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ArrayElementsContext ------------------------------------------------------------------

CrySLParser::ArrayElementsContext::ArrayElementsContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

CrySLParser::ConsPredContext* CrySLParser::ArrayElementsContext::consPred() {
  return getRuleContext<CrySLParser::ConsPredContext>(0);
}


size_t CrySLParser::ArrayElementsContext::getRuleIndex() const {
  return CrySLParser::RuleArrayElements;
}


CrySLParser::ArrayElementsContext* CrySLParser::arrayElements() {
  ArrayElementsContext *_localctx = _tracker.createInstance<ArrayElementsContext>(_ctx, getState());
  enterRule(_localctx, 48, CrySLParser::RuleArrayElements);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    setState(339);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case CrySLParser::T__41: {
        enterOuterAlt(_localctx, 1);
        setState(333);
        dynamic_cast<ArrayElementsContext *>(_localctx)->el = match(CrySLParser::T__41);
        setState(334);
        match(CrySLParser::T__25);
        setState(335);
        consPred();
        setState(336);
        match(CrySLParser::T__26);
        break;
      }

      case CrySLParser::T__4:
      case CrySLParser::T__17:
      case CrySLParser::T__18:
      case CrySLParser::T__19:
      case CrySLParser::T__20:
      case CrySLParser::T__21:
      case CrySLParser::Int:
      case CrySLParser::Bool:
      case CrySLParser::String:
      case CrySLParser::Ident: {
        enterOuterAlt(_localctx, 2);
        setState(338);
        consPred();
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- LitListContext ------------------------------------------------------------------

CrySLParser::LitListContext::LitListContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<CrySLParser::LiteralContext *> CrySLParser::LitListContext::literal() {
  return getRuleContexts<CrySLParser::LiteralContext>();
}

CrySLParser::LiteralContext* CrySLParser::LitListContext::literal(size_t i) {
  return getRuleContext<CrySLParser::LiteralContext>(i);
}


size_t CrySLParser::LitListContext::getRuleIndex() const {
  return CrySLParser::RuleLitList;
}


CrySLParser::LitListContext* CrySLParser::litList() {
  LitListContext *_localctx = _tracker.createInstance<LitListContext>(_ctx, getState());
  enterRule(_localctx, 50, CrySLParser::RuleLitList);
  size_t _la = 0;

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(341);
    literal();
    setState(349);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == CrySLParser::T__9) {
      setState(342);
      match(CrySLParser::T__9);
      setState(345);
      _errHandler->sync(this);
      switch (_input->LA(1)) {
        case CrySLParser::Int:
        case CrySLParser::Bool:
        case CrySLParser::String: {
          setState(343);
          literal();
          break;
        }

        case CrySLParser::T__42: {
          setState(344);
          dynamic_cast<LitListContext *>(_localctx)->ellipsis = match(CrySLParser::T__42);
          break;
        }

      default:
        throw NoViableAltException(this);
      }
      setState(351);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- EventsContext ------------------------------------------------------------------

CrySLParser::EventsContext::EventsContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<CrySLParser::EventsOccurenceContext *> CrySLParser::EventsContext::eventsOccurence() {
  return getRuleContexts<CrySLParser::EventsOccurenceContext>();
}

CrySLParser::EventsOccurenceContext* CrySLParser::EventsContext::eventsOccurence(size_t i) {
  return getRuleContext<CrySLParser::EventsOccurenceContext>(i);
}


size_t CrySLParser::EventsContext::getRuleIndex() const {
  return CrySLParser::RuleEvents;
}


CrySLParser::EventsContext* CrySLParser::events() {
  EventsContext *_localctx = _tracker.createInstance<EventsContext>(_ctx, getState());
  enterRule(_localctx, 52, CrySLParser::RuleEvents);
  size_t _la = 0;

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(352);
    match(CrySLParser::T__43);
    setState(354); 
    _errHandler->sync(this);
    _la = _input->LA(1);
    do {
      setState(353);
      eventsOccurence();
      setState(356); 
      _errHandler->sync(this);
      _la = _input->LA(1);
    } while (_la == CrySLParser::Ident);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- EventsOccurenceContext ------------------------------------------------------------------

CrySLParser::EventsOccurenceContext::EventsOccurenceContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<tree::TerminalNode *> CrySLParser::EventsOccurenceContext::Ident() {
  return getTokens(CrySLParser::Ident);
}

tree::TerminalNode* CrySLParser::EventsOccurenceContext::Ident(size_t i) {
  return getToken(CrySLParser::Ident, i);
}

CrySLParser::ParametersListContext* CrySLParser::EventsOccurenceContext::parametersList() {
  return getRuleContext<CrySLParser::ParametersListContext>(0);
}


size_t CrySLParser::EventsOccurenceContext::getRuleIndex() const {
  return CrySLParser::RuleEventsOccurence;
}


CrySLParser::EventsOccurenceContext* CrySLParser::eventsOccurence() {
  EventsOccurenceContext *_localctx = _tracker.createInstance<EventsOccurenceContext>(_ctx, getState());
  enterRule(_localctx, 54, CrySLParser::RuleEventsOccurence);
  size_t _la = 0;

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(358);
    dynamic_cast<EventsOccurenceContext *>(_localctx)->eventName = match(CrySLParser::Ident);
    setState(359);
    match(CrySLParser::T__44);
    setState(360);
    dynamic_cast<EventsOccurenceContext *>(_localctx)->methodName = match(CrySLParser::Ident);
    setState(361);
    match(CrySLParser::T__25);
    setState(363);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & ((1ULL << CrySLParser::T__4)
      | (1ULL << CrySLParser::T__12)
      | (1ULL << CrySLParser::T__13)
      | (1ULL << CrySLParser::Ident))) != 0)) {
      setState(362);
      parametersList();
    }
    setState(365);
    match(CrySLParser::T__26);
    setState(366);
    match(CrySLParser::T__3);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ParametersListContext ------------------------------------------------------------------

CrySLParser::ParametersListContext::ParametersListContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<CrySLParser::ParamContext *> CrySLParser::ParametersListContext::param() {
  return getRuleContexts<CrySLParser::ParamContext>();
}

CrySLParser::ParamContext* CrySLParser::ParametersListContext::param(size_t i) {
  return getRuleContext<CrySLParser::ParamContext>(i);
}


size_t CrySLParser::ParametersListContext::getRuleIndex() const {
  return CrySLParser::RuleParametersList;
}


CrySLParser::ParametersListContext* CrySLParser::parametersList() {
  ParametersListContext *_localctx = _tracker.createInstance<ParametersListContext>(_ctx, getState());
  enterRule(_localctx, 56, CrySLParser::RuleParametersList);
  size_t _la = 0;

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(368);
    param();
    setState(373);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == CrySLParser::T__9) {
      setState(369);
      match(CrySLParser::T__9);
      setState(370);
      param();
      setState(375);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ParamContext ------------------------------------------------------------------

CrySLParser::ParamContext::ParamContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

CrySLParser::MemberAccessContext* CrySLParser::ParamContext::memberAccess() {
  return getRuleContext<CrySLParser::MemberAccessContext>(0);
}


size_t CrySLParser::ParamContext::getRuleIndex() const {
  return CrySLParser::RuleParam;
}


CrySLParser::ParamContext* CrySLParser::param() {
  ParamContext *_localctx = _tracker.createInstance<ParamContext>(_ctx, getState());
  enterRule(_localctx, 58, CrySLParser::RuleParam);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    setState(379);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case CrySLParser::T__4:
      case CrySLParser::Ident: {
        enterOuterAlt(_localctx, 1);
        setState(376);
        memberAccess();
        break;
      }

      case CrySLParser::T__12: {
        enterOuterAlt(_localctx, 2);
        setState(377);
        dynamic_cast<ParamContext *>(_localctx)->thisPtr = match(CrySLParser::T__12);
        break;
      }

      case CrySLParser::T__13: {
        enterOuterAlt(_localctx, 3);
        setState(378);
        dynamic_cast<ParamContext *>(_localctx)->wildCard = match(CrySLParser::T__13);
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- OrderContext ------------------------------------------------------------------

CrySLParser::OrderContext::OrderContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

CrySLParser::OrderSequenceContext* CrySLParser::OrderContext::orderSequence() {
  return getRuleContext<CrySLParser::OrderSequenceContext>(0);
}


size_t CrySLParser::OrderContext::getRuleIndex() const {
  return CrySLParser::RuleOrder;
}


CrySLParser::OrderContext* CrySLParser::order() {
  OrderContext *_localctx = _tracker.createInstance<OrderContext>(_ctx, getState());
  enterRule(_localctx, 60, CrySLParser::RuleOrder);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(381);
    match(CrySLParser::T__45);
    setState(382);
    orderSequence();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- OrderSequenceContext ------------------------------------------------------------------

CrySLParser::OrderSequenceContext::OrderSequenceContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<CrySLParser::SimpleOrderContext *> CrySLParser::OrderSequenceContext::simpleOrder() {
  return getRuleContexts<CrySLParser::SimpleOrderContext>();
}

CrySLParser::SimpleOrderContext* CrySLParser::OrderSequenceContext::simpleOrder(size_t i) {
  return getRuleContext<CrySLParser::SimpleOrderContext>(i);
}


size_t CrySLParser::OrderSequenceContext::getRuleIndex() const {
  return CrySLParser::RuleOrderSequence;
}


CrySLParser::OrderSequenceContext* CrySLParser::orderSequence() {
  OrderSequenceContext *_localctx = _tracker.createInstance<OrderSequenceContext>(_ctx, getState());
  enterRule(_localctx, 62, CrySLParser::RuleOrderSequence);
  size_t _la = 0;

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(384);
    simpleOrder();
    setState(389);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == CrySLParser::T__9) {
      setState(385);
      match(CrySLParser::T__9);
      setState(386);
      simpleOrder();
      setState(391);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- SimpleOrderContext ------------------------------------------------------------------

CrySLParser::SimpleOrderContext::SimpleOrderContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<CrySLParser::UnorderdSymbolsContext *> CrySLParser::SimpleOrderContext::unorderdSymbols() {
  return getRuleContexts<CrySLParser::UnorderdSymbolsContext>();
}

CrySLParser::UnorderdSymbolsContext* CrySLParser::SimpleOrderContext::unorderdSymbols(size_t i) {
  return getRuleContext<CrySLParser::UnorderdSymbolsContext>(i);
}


size_t CrySLParser::SimpleOrderContext::getRuleIndex() const {
  return CrySLParser::RuleSimpleOrder;
}


CrySLParser::SimpleOrderContext* CrySLParser::simpleOrder() {
  SimpleOrderContext *_localctx = _tracker.createInstance<SimpleOrderContext>(_ctx, getState());
  enterRule(_localctx, 64, CrySLParser::RuleSimpleOrder);
  size_t _la = 0;

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(392);
    unorderdSymbols();
    setState(397);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == CrySLParser::T__46) {
      setState(393);
      match(CrySLParser::T__46);
      setState(394);
      unorderdSymbols();
      setState(399);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- UnorderdSymbolsContext ------------------------------------------------------------------

CrySLParser::UnorderdSymbolsContext::UnorderdSymbolsContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<CrySLParser::PrimaryContext *> CrySLParser::UnorderdSymbolsContext::primary() {
  return getRuleContexts<CrySLParser::PrimaryContext>();
}

CrySLParser::PrimaryContext* CrySLParser::UnorderdSymbolsContext::primary(size_t i) {
  return getRuleContext<CrySLParser::PrimaryContext>(i);
}

std::vector<tree::TerminalNode *> CrySLParser::UnorderdSymbolsContext::Int() {
  return getTokens(CrySLParser::Int);
}

tree::TerminalNode* CrySLParser::UnorderdSymbolsContext::Int(size_t i) {
  return getToken(CrySLParser::Int, i);
}


size_t CrySLParser::UnorderdSymbolsContext::getRuleIndex() const {
  return CrySLParser::RuleUnorderdSymbols;
}


CrySLParser::UnorderdSymbolsContext* CrySLParser::unorderdSymbols() {
  UnorderdSymbolsContext *_localctx = _tracker.createInstance<UnorderdSymbolsContext>(_ctx, getState());
  enterRule(_localctx, 66, CrySLParser::RuleUnorderdSymbols);
  size_t _la = 0;

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(400);
    primary();
    setState(416);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == CrySLParser::T__47) {
      setState(403); 
      _errHandler->sync(this);
      _la = _input->LA(1);
      do {
        setState(401);
        match(CrySLParser::T__47);
        setState(402);
        primary();
        setState(405); 
        _errHandler->sync(this);
        _la = _input->LA(1);
      } while (_la == CrySLParser::T__47);
      setState(414);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == CrySLParser::T__48

      || _la == CrySLParser::Int) {
        setState(408);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (_la == CrySLParser::Int) {
          setState(407);
          dynamic_cast<UnorderdSymbolsContext *>(_localctx)->lower = match(CrySLParser::Int);
        }
        setState(410);
        match(CrySLParser::T__48);
        setState(412);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (_la == CrySLParser::Int) {
          setState(411);
          dynamic_cast<UnorderdSymbolsContext *>(_localctx)->upper = match(CrySLParser::Int);
        }
      }
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- PrimaryContext ------------------------------------------------------------------

CrySLParser::PrimaryContext::PrimaryContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* CrySLParser::PrimaryContext::Ident() {
  return getToken(CrySLParser::Ident, 0);
}

CrySLParser::OrderSequenceContext* CrySLParser::PrimaryContext::orderSequence() {
  return getRuleContext<CrySLParser::OrderSequenceContext>(0);
}


size_t CrySLParser::PrimaryContext::getRuleIndex() const {
  return CrySLParser::RulePrimary;
}


CrySLParser::PrimaryContext* CrySLParser::primary() {
  PrimaryContext *_localctx = _tracker.createInstance<PrimaryContext>(_ctx, getState());
  enterRule(_localctx, 68, CrySLParser::RulePrimary);
  size_t _la = 0;

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    setState(428);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case CrySLParser::Ident: {
        enterOuterAlt(_localctx, 1);
        setState(418);
        dynamic_cast<PrimaryContext *>(_localctx)->eventName = match(CrySLParser::Ident);
        setState(420);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if ((((_la & ~ 0x3fULL) == 0) &&
          ((1ULL << _la) & ((1ULL << CrySLParser::T__4)
          | (1ULL << CrySLParser::T__29)
          | (1ULL << CrySLParser::T__49))) != 0)) {
          setState(419);
          dynamic_cast<PrimaryContext *>(_localctx)->elementop = _input->LT(1);
          _la = _input->LA(1);
          if (!((((_la & ~ 0x3fULL) == 0) &&
            ((1ULL << _la) & ((1ULL << CrySLParser::T__4)
            | (1ULL << CrySLParser::T__29)
            | (1ULL << CrySLParser::T__49))) != 0))) {
            dynamic_cast<PrimaryContext *>(_localctx)->elementop = _errHandler->recoverInline(this);
          }
          else {
            _errHandler->reportMatch(this);
            consume();
          }
        }
        break;
      }

      case CrySLParser::T__25: {
        enterOuterAlt(_localctx, 2);
        setState(422);
        match(CrySLParser::T__25);
        setState(423);
        orderSequence();
        setState(424);
        match(CrySLParser::T__26);
        setState(426);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if ((((_la & ~ 0x3fULL) == 0) &&
          ((1ULL << _la) & ((1ULL << CrySLParser::T__4)
          | (1ULL << CrySLParser::T__29)
          | (1ULL << CrySLParser::T__49))) != 0)) {
          setState(425);
          dynamic_cast<PrimaryContext *>(_localctx)->elementop = _input->LT(1);
          _la = _input->LA(1);
          if (!((((_la & ~ 0x3fULL) == 0) &&
            ((1ULL << _la) & ((1ULL << CrySLParser::T__4)
            | (1ULL << CrySLParser::T__29)
            | (1ULL << CrySLParser::T__49))) != 0))) {
            dynamic_cast<PrimaryContext *>(_localctx)->elementop = _errHandler->recoverInline(this);
          }
          else {
            _errHandler->reportMatch(this);
            consume();
          }
        }
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- NegatesContext ------------------------------------------------------------------

CrySLParser::NegatesContext::NegatesContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<CrySLParser::NegatesOccurenceContext *> CrySLParser::NegatesContext::negatesOccurence() {
  return getRuleContexts<CrySLParser::NegatesOccurenceContext>();
}

CrySLParser::NegatesOccurenceContext* CrySLParser::NegatesContext::negatesOccurence(size_t i) {
  return getRuleContext<CrySLParser::NegatesOccurenceContext>(i);
}


size_t CrySLParser::NegatesContext::getRuleIndex() const {
  return CrySLParser::RuleNegates;
}


CrySLParser::NegatesContext* CrySLParser::negates() {
  NegatesContext *_localctx = _tracker.createInstance<NegatesContext>(_ctx, getState());
  enterRule(_localctx, 70, CrySLParser::RuleNegates);
  size_t _la = 0;

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(430);
    match(CrySLParser::T__50);
    setState(432); 
    _errHandler->sync(this);
    _la = _input->LA(1);
    do {
      setState(431);
      negatesOccurence();
      setState(434); 
      _errHandler->sync(this);
      _la = _input->LA(1);
    } while (_la == CrySLParser::Ident);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- NegatesOccurenceContext ------------------------------------------------------------------

CrySLParser::NegatesOccurenceContext::NegatesOccurenceContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

CrySLParser::EnsPredContext* CrySLParser::NegatesOccurenceContext::ensPred() {
  return getRuleContext<CrySLParser::EnsPredContext>(0);
}


size_t CrySLParser::NegatesOccurenceContext::getRuleIndex() const {
  return CrySLParser::RuleNegatesOccurence;
}


CrySLParser::NegatesOccurenceContext* CrySLParser::negatesOccurence() {
  NegatesOccurenceContext *_localctx = _tracker.createInstance<NegatesOccurenceContext>(_ctx, getState());
  enterRule(_localctx, 72, CrySLParser::RuleNegatesOccurence);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(436);
    ensPred();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ForbiddenContext ------------------------------------------------------------------

CrySLParser::ForbiddenContext::ForbiddenContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<CrySLParser::ForbiddenOccurenceContext *> CrySLParser::ForbiddenContext::forbiddenOccurence() {
  return getRuleContexts<CrySLParser::ForbiddenOccurenceContext>();
}

CrySLParser::ForbiddenOccurenceContext* CrySLParser::ForbiddenContext::forbiddenOccurence(size_t i) {
  return getRuleContext<CrySLParser::ForbiddenOccurenceContext>(i);
}


size_t CrySLParser::ForbiddenContext::getRuleIndex() const {
  return CrySLParser::RuleForbidden;
}


CrySLParser::ForbiddenContext* CrySLParser::forbidden() {
  ForbiddenContext *_localctx = _tracker.createInstance<ForbiddenContext>(_ctx, getState());
  enterRule(_localctx, 74, CrySLParser::RuleForbidden);
  size_t _la = 0;

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(438);
    match(CrySLParser::T__51);
    setState(440); 
    _errHandler->sync(this);
    _la = _input->LA(1);
    do {
      setState(439);
      forbiddenOccurence();
      setState(442); 
      _errHandler->sync(this);
      _la = _input->LA(1);
    } while (_la == CrySLParser::Ident);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ForbiddenOccurenceContext ------------------------------------------------------------------

CrySLParser::ForbiddenOccurenceContext::ForbiddenOccurenceContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

CrySLParser::FqnContext* CrySLParser::ForbiddenOccurenceContext::fqn() {
  return getRuleContext<CrySLParser::FqnContext>(0);
}

tree::TerminalNode* CrySLParser::ForbiddenOccurenceContext::Ident() {
  return getToken(CrySLParser::Ident, 0);
}


size_t CrySLParser::ForbiddenOccurenceContext::getRuleIndex() const {
  return CrySLParser::RuleForbiddenOccurence;
}


CrySLParser::ForbiddenOccurenceContext* CrySLParser::forbiddenOccurence() {
  ForbiddenOccurenceContext *_localctx = _tracker.createInstance<ForbiddenOccurenceContext>(_ctx, getState());
  enterRule(_localctx, 76, CrySLParser::RuleForbiddenOccurence);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(444);
    dynamic_cast<ForbiddenOccurenceContext *>(_localctx)->methodName = fqn();

    setState(445);
    match(CrySLParser::T__11);
    setState(446);
    dynamic_cast<ForbiddenOccurenceContext *>(_localctx)->eventName = match(CrySLParser::Ident);
    setState(448);
    match(CrySLParser::T__3);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- FqnContext ------------------------------------------------------------------

CrySLParser::FqnContext::FqnContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

CrySLParser::QualifiedNameContext* CrySLParser::FqnContext::qualifiedName() {
  return getRuleContext<CrySLParser::QualifiedNameContext>(0);
}

CrySLParser::TypeNameListContext* CrySLParser::FqnContext::typeNameList() {
  return getRuleContext<CrySLParser::TypeNameListContext>(0);
}


size_t CrySLParser::FqnContext::getRuleIndex() const {
  return CrySLParser::RuleFqn;
}


CrySLParser::FqnContext* CrySLParser::fqn() {
  FqnContext *_localctx = _tracker.createInstance<FqnContext>(_ctx, getState());
  enterRule(_localctx, 78, CrySLParser::RuleFqn);
  size_t _la = 0;

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(450);
    qualifiedName();
    setState(451);
    match(CrySLParser::T__25);
    setState(453);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == CrySLParser::Ident) {
      setState(452);
      typeNameList();
    }
    setState(455);
    match(CrySLParser::T__26);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- TypeNameListContext ------------------------------------------------------------------

CrySLParser::TypeNameListContext::TypeNameListContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<CrySLParser::TypeNameContext *> CrySLParser::TypeNameListContext::typeName() {
  return getRuleContexts<CrySLParser::TypeNameContext>();
}

CrySLParser::TypeNameContext* CrySLParser::TypeNameListContext::typeName(size_t i) {
  return getRuleContext<CrySLParser::TypeNameContext>(i);
}


size_t CrySLParser::TypeNameListContext::getRuleIndex() const {
  return CrySLParser::RuleTypeNameList;
}


CrySLParser::TypeNameListContext* CrySLParser::typeNameList() {
  TypeNameListContext *_localctx = _tracker.createInstance<TypeNameListContext>(_ctx, getState());
  enterRule(_localctx, 80, CrySLParser::RuleTypeNameList);
  size_t _la = 0;

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(457);
    typeName();
    setState(462);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == CrySLParser::T__9) {
      setState(458);
      match(CrySLParser::T__9);
      setState(459);
      typeName();
      setState(464);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

bool CrySLParser::sempred(RuleContext *context, size_t ruleIndex, size_t predicateIndex) {
  switch (ruleIndex) {
    case 8: return reqPredSempred(dynamic_cast<ReqPredContext *>(context), predicateIndex);
    case 21: return constrSempred(dynamic_cast<ConstrContext *>(context), predicateIndex);

  default:
    break;
  }
  return true;
}

bool CrySLParser::reqPredSempred(ReqPredContext *_localctx, size_t predicateIndex) {
  switch (predicateIndex) {
    case 0: return precpred(_ctx, 3);
    case 1: return precpred(_ctx, 2);

  default:
    break;
  }
  return true;
}

bool CrySLParser::constrSempred(ConstrContext *_localctx, size_t predicateIndex) {
  switch (predicateIndex) {
    case 2: return precpred(_ctx, 8);
    case 3: return precpred(_ctx, 7);
    case 4: return precpred(_ctx, 6);
    case 5: return precpred(_ctx, 5);
    case 6: return precpred(_ctx, 4);
    case 7: return precpred(_ctx, 3);
    case 8: return precpred(_ctx, 2);
    case 9: return precpred(_ctx, 1);

  default:
    break;
  }
  return true;
}

// Static vars and initialization.
std::vector<dfa::DFA> CrySLParser::_decisionToDFA;
atn::PredictionContextCache CrySLParser::_sharedContextCache;

// We own the ATN which in turn owns the ATN states.
atn::ATN CrySLParser::_atn;
std::vector<uint16_t> CrySLParser::_serializedATN;

std::vector<std::string> CrySLParser::_ruleNames = {
  "domainModel", "spec", "qualifiedName", "objects", "objectDecl", "typeName", 
  "array", "requiresBlock", "reqPred", "reqPredLit", "pred", "suParList", 
  "suPar", "consPred", "literalExpr", "literal", "memberAccess", "preDefinedPredicate", 
  "ensures", "ensPred", "constraints", "constr", "comparingRelOperator", 
  "cons", "arrayElements", "litList", "events", "eventsOccurence", "parametersList", 
  "param", "order", "orderSequence", "simpleOrder", "unorderdSymbols", "primary", 
  "negates", "negatesOccurence", "forbidden", "forbiddenOccurence", "fqn", 
  "typeNameList"
};

std::vector<std::string> CrySLParser::_literalNames = {
  "", "'SPEC'", "'::'", "'OBJECTS'", "';'", "'*'", "'['", "']'", "'REQUIRES'", 
  "'!'", "','", "'||'", "'=>'", "'this'", "'_'", "'^'", "'.'", "'->'", "'neverTypeOf'", 
  "'noCallTo'", "'callTo'", "'notHardCoded'", "'length'", "'ENSURES'", "'after'", 
  "'CONSTRAINTS'", "'('", "')'", "'/'", "'%'", "'+'", "'-'", "'=='", "'!='", 
  "'&&'", "'<'", "'<='", "'>='", "'>'", "'in'", "'{'", "'}'", "'elements'", 
  "'...'", "'EVENTS'", "':'", "'ORDER'", "'|'", "'~'", "'#'", "'?'", "'NEGATES'", 
  "'FORBIDDEN'"
};

std::vector<std::string> CrySLParser::_symbolicNames = {
  "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", 
  "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", 
  "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "Int", 
  "Float", "Double", "Char", "Bool", "String", "Ident", "COMMENT", "LINE_COMMENT", 
  "WS"
};

dfa::Vocabulary CrySLParser::_vocabulary(_literalNames, _symbolicNames);

std::vector<std::string> CrySLParser::_tokenNames;

CrySLParser::Initializer::Initializer() {
	for (size_t i = 0; i < _symbolicNames.size(); ++i) {
		std::string name = _vocabulary.getLiteralName(i);
		if (name.empty()) {
			name = _vocabulary.getSymbolicName(i);
		}

		if (name.empty()) {
			_tokenNames.push_back("<INVALID>");
		} else {
      _tokenNames.push_back(name);
    }
	}

  _serializedATN = {
    0x3, 0x608b, 0xa72a, 0x8133, 0xb9ed, 0x417c, 0x3be7, 0x7786, 0x5964, 
    0x3, 0x40, 0x1d4, 0x4, 0x2, 0x9, 0x2, 0x4, 0x3, 0x9, 0x3, 0x4, 0x4, 
    0x9, 0x4, 0x4, 0x5, 0x9, 0x5, 0x4, 0x6, 0x9, 0x6, 0x4, 0x7, 0x9, 0x7, 
    0x4, 0x8, 0x9, 0x8, 0x4, 0x9, 0x9, 0x9, 0x4, 0xa, 0x9, 0xa, 0x4, 0xb, 
    0x9, 0xb, 0x4, 0xc, 0x9, 0xc, 0x4, 0xd, 0x9, 0xd, 0x4, 0xe, 0x9, 0xe, 
    0x4, 0xf, 0x9, 0xf, 0x4, 0x10, 0x9, 0x10, 0x4, 0x11, 0x9, 0x11, 0x4, 
    0x12, 0x9, 0x12, 0x4, 0x13, 0x9, 0x13, 0x4, 0x14, 0x9, 0x14, 0x4, 0x15, 
    0x9, 0x15, 0x4, 0x16, 0x9, 0x16, 0x4, 0x17, 0x9, 0x17, 0x4, 0x18, 0x9, 
    0x18, 0x4, 0x19, 0x9, 0x19, 0x4, 0x1a, 0x9, 0x1a, 0x4, 0x1b, 0x9, 0x1b, 
    0x4, 0x1c, 0x9, 0x1c, 0x4, 0x1d, 0x9, 0x1d, 0x4, 0x1e, 0x9, 0x1e, 0x4, 
    0x1f, 0x9, 0x1f, 0x4, 0x20, 0x9, 0x20, 0x4, 0x21, 0x9, 0x21, 0x4, 0x22, 
    0x9, 0x22, 0x4, 0x23, 0x9, 0x23, 0x4, 0x24, 0x9, 0x24, 0x4, 0x25, 0x9, 
    0x25, 0x4, 0x26, 0x9, 0x26, 0x4, 0x27, 0x9, 0x27, 0x4, 0x28, 0x9, 0x28, 
    0x4, 0x29, 0x9, 0x29, 0x4, 0x2a, 0x9, 0x2a, 0x3, 0x2, 0x3, 0x2, 0x3, 
    0x2, 0x5, 0x2, 0x58, 0xa, 0x2, 0x3, 0x2, 0x3, 0x2, 0x3, 0x2, 0x5, 0x2, 
    0x5d, 0xa, 0x2, 0x3, 0x2, 0x5, 0x2, 0x60, 0xa, 0x2, 0x3, 0x2, 0x5, 0x2, 
    0x63, 0xa, 0x2, 0x3, 0x2, 0x5, 0x2, 0x66, 0xa, 0x2, 0x3, 0x2, 0x3, 0x2, 
    0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x4, 0x3, 0x4, 0x3, 0x4, 0x7, 0x4, 
    0x70, 0xa, 0x4, 0xc, 0x4, 0xe, 0x4, 0x73, 0xb, 0x4, 0x3, 0x5, 0x3, 0x5, 
    0x7, 0x5, 0x77, 0xa, 0x5, 0xc, 0x5, 0xe, 0x5, 0x7a, 0xb, 0x5, 0x3, 0x6, 
    0x3, 0x6, 0x3, 0x6, 0x7, 0x6, 0x7f, 0xa, 0x6, 0xc, 0x6, 0xe, 0x6, 0x82, 
    0xb, 0x6, 0x3, 0x6, 0x3, 0x6, 0x3, 0x7, 0x3, 0x7, 0x7, 0x7, 0x88, 0xa, 
    0x7, 0xc, 0x7, 0xe, 0x7, 0x8b, 0xb, 0x7, 0x3, 0x8, 0x3, 0x8, 0x3, 0x8, 
    0x3, 0x8, 0x3, 0x9, 0x3, 0x9, 0x3, 0x9, 0x3, 0x9, 0x7, 0x9, 0x95, 0xa, 
    0x9, 0xc, 0x9, 0xe, 0x9, 0x98, 0xb, 0x9, 0x3, 0xa, 0x3, 0xa, 0x5, 0xa, 
    0x9c, 0xa, 0xa, 0x3, 0xa, 0x3, 0xa, 0x3, 0xa, 0x3, 0xa, 0x3, 0xa, 0x5, 
    0xa, 0xa3, 0xa, 0xa, 0x3, 0xa, 0x3, 0xa, 0x3, 0xa, 0x3, 0xa, 0x3, 0xa, 
    0x3, 0xa, 0x7, 0xa, 0xab, 0xa, 0xa, 0xc, 0xa, 0xe, 0xa, 0xae, 0xb, 0xa, 
    0x3, 0xb, 0x3, 0xb, 0x3, 0xc, 0x3, 0xc, 0x3, 0xc, 0x5, 0xc, 0xb5, 0xa, 
    0xc, 0x3, 0xc, 0x3, 0xc, 0x3, 0xd, 0x3, 0xd, 0x3, 0xd, 0x7, 0xd, 0xbc, 
    0xa, 0xd, 0xc, 0xd, 0xe, 0xd, 0xbf, 0xb, 0xd, 0x3, 0xe, 0x3, 0xe, 0x3, 
    0xe, 0x5, 0xe, 0xc4, 0xa, 0xe, 0x3, 0xf, 0x3, 0xf, 0x3, 0x10, 0x3, 0x10, 
    0x3, 0x10, 0x5, 0x10, 0xcb, 0xa, 0x10, 0x3, 0x11, 0x3, 0x11, 0x3, 0x11, 
    0x3, 0x11, 0x3, 0x11, 0x3, 0x11, 0x5, 0x11, 0xd3, 0xa, 0x11, 0x3, 0x12, 
    0x3, 0x12, 0x3, 0x12, 0x3, 0x12, 0x3, 0x12, 0x3, 0x12, 0x3, 0x12, 0x3, 
    0x12, 0x3, 0x12, 0x5, 0x12, 0xde, 0xa, 0x12, 0x3, 0x13, 0x3, 0x13, 0x3, 
    0x13, 0x3, 0x13, 0x3, 0x13, 0x3, 0x13, 0x3, 0x13, 0x3, 0x13, 0x3, 0x13, 
    0x3, 0x13, 0x3, 0x13, 0x3, 0x13, 0x3, 0x13, 0x3, 0x13, 0x3, 0x13, 0x3, 
    0x13, 0x3, 0x13, 0x3, 0x13, 0x3, 0x13, 0x3, 0x13, 0x3, 0x13, 0x3, 0x13, 
    0x3, 0x13, 0x3, 0x13, 0x3, 0x13, 0x5, 0x13, 0xf9, 0xa, 0x13, 0x3, 0x14, 
    0x3, 0x14, 0x3, 0x14, 0x3, 0x14, 0x6, 0x14, 0xff, 0xa, 0x14, 0xd, 0x14, 
    0xe, 0x14, 0x100, 0x3, 0x15, 0x3, 0x15, 0x3, 0x15, 0x5, 0x15, 0x106, 
    0xa, 0x15, 0x3, 0x16, 0x3, 0x16, 0x3, 0x16, 0x3, 0x16, 0x6, 0x16, 0x10c, 
    0xa, 0x16, 0xd, 0x16, 0xe, 0x16, 0x10d, 0x3, 0x17, 0x3, 0x17, 0x3, 0x17, 
    0x3, 0x17, 0x3, 0x17, 0x3, 0x17, 0x3, 0x17, 0x3, 0x17, 0x5, 0x17, 0x118, 
    0xa, 0x17, 0x3, 0x17, 0x3, 0x17, 0x3, 0x17, 0x5, 0x17, 0x11d, 0xa, 0x17, 
    0x3, 0x17, 0x3, 0x17, 0x3, 0x17, 0x3, 0x17, 0x3, 0x17, 0x3, 0x17, 0x3, 
    0x17, 0x5, 0x17, 0x126, 0xa, 0x17, 0x3, 0x17, 0x3, 0x17, 0x3, 0x17, 
    0x3, 0x17, 0x3, 0x17, 0x3, 0x17, 0x3, 0x17, 0x3, 0x17, 0x5, 0x17, 0x130, 
    0xa, 0x17, 0x3, 0x17, 0x3, 0x17, 0x3, 0x17, 0x3, 0x17, 0x3, 0x17, 0x3, 
    0x17, 0x3, 0x17, 0x3, 0x17, 0x3, 0x17, 0x3, 0x17, 0x7, 0x17, 0x13c, 
    0xa, 0x17, 0xc, 0x17, 0xe, 0x17, 0x13f, 0xb, 0x17, 0x3, 0x18, 0x3, 0x18, 
    0x3, 0x18, 0x3, 0x18, 0x5, 0x18, 0x145, 0xa, 0x18, 0x3, 0x19, 0x3, 0x19, 
    0x3, 0x19, 0x3, 0x19, 0x3, 0x19, 0x3, 0x19, 0x3, 0x19, 0x5, 0x19, 0x14e, 
    0xa, 0x19, 0x3, 0x1a, 0x3, 0x1a, 0x3, 0x1a, 0x3, 0x1a, 0x3, 0x1a, 0x3, 
    0x1a, 0x5, 0x1a, 0x156, 0xa, 0x1a, 0x3, 0x1b, 0x3, 0x1b, 0x3, 0x1b, 
    0x3, 0x1b, 0x5, 0x1b, 0x15c, 0xa, 0x1b, 0x7, 0x1b, 0x15e, 0xa, 0x1b, 
    0xc, 0x1b, 0xe, 0x1b, 0x161, 0xb, 0x1b, 0x3, 0x1c, 0x3, 0x1c, 0x6, 0x1c, 
    0x165, 0xa, 0x1c, 0xd, 0x1c, 0xe, 0x1c, 0x166, 0x3, 0x1d, 0x3, 0x1d, 
    0x3, 0x1d, 0x3, 0x1d, 0x3, 0x1d, 0x5, 0x1d, 0x16e, 0xa, 0x1d, 0x3, 0x1d, 
    0x3, 0x1d, 0x3, 0x1d, 0x3, 0x1e, 0x3, 0x1e, 0x3, 0x1e, 0x7, 0x1e, 0x176, 
    0xa, 0x1e, 0xc, 0x1e, 0xe, 0x1e, 0x179, 0xb, 0x1e, 0x3, 0x1f, 0x3, 0x1f, 
    0x3, 0x1f, 0x5, 0x1f, 0x17e, 0xa, 0x1f, 0x3, 0x20, 0x3, 0x20, 0x3, 0x20, 
    0x3, 0x21, 0x3, 0x21, 0x3, 0x21, 0x7, 0x21, 0x186, 0xa, 0x21, 0xc, 0x21, 
    0xe, 0x21, 0x189, 0xb, 0x21, 0x3, 0x22, 0x3, 0x22, 0x3, 0x22, 0x7, 0x22, 
    0x18e, 0xa, 0x22, 0xc, 0x22, 0xe, 0x22, 0x191, 0xb, 0x22, 0x3, 0x23, 
    0x3, 0x23, 0x3, 0x23, 0x6, 0x23, 0x196, 0xa, 0x23, 0xd, 0x23, 0xe, 0x23, 
    0x197, 0x3, 0x23, 0x5, 0x23, 0x19b, 0xa, 0x23, 0x3, 0x23, 0x3, 0x23, 
    0x5, 0x23, 0x19f, 0xa, 0x23, 0x5, 0x23, 0x1a1, 0xa, 0x23, 0x5, 0x23, 
    0x1a3, 0xa, 0x23, 0x3, 0x24, 0x3, 0x24, 0x5, 0x24, 0x1a7, 0xa, 0x24, 
    0x3, 0x24, 0x3, 0x24, 0x3, 0x24, 0x3, 0x24, 0x5, 0x24, 0x1ad, 0xa, 0x24, 
    0x5, 0x24, 0x1af, 0xa, 0x24, 0x3, 0x25, 0x3, 0x25, 0x6, 0x25, 0x1b3, 
    0xa, 0x25, 0xd, 0x25, 0xe, 0x25, 0x1b4, 0x3, 0x26, 0x3, 0x26, 0x3, 0x27, 
    0x3, 0x27, 0x6, 0x27, 0x1bb, 0xa, 0x27, 0xd, 0x27, 0xe, 0x27, 0x1bc, 
    0x3, 0x28, 0x3, 0x28, 0x3, 0x28, 0x3, 0x28, 0x3, 0x28, 0x3, 0x28, 0x3, 
    0x29, 0x3, 0x29, 0x3, 0x29, 0x5, 0x29, 0x1c8, 0xa, 0x29, 0x3, 0x29, 
    0x3, 0x29, 0x3, 0x2a, 0x3, 0x2a, 0x3, 0x2a, 0x7, 0x2a, 0x1cf, 0xa, 0x2a, 
    0xc, 0x2a, 0xe, 0x2a, 0x1d2, 0xb, 0x2a, 0x3, 0x2a, 0x2, 0x4, 0x12, 0x2c, 
    0x2b, 0x2, 0x4, 0x6, 0x8, 0xa, 0xc, 0xe, 0x10, 0x12, 0x14, 0x16, 0x18, 
    0x1a, 0x1c, 0x1e, 0x20, 0x22, 0x24, 0x26, 0x28, 0x2a, 0x2c, 0x2e, 0x30, 
    0x32, 0x34, 0x36, 0x38, 0x3a, 0x3c, 0x3e, 0x40, 0x42, 0x44, 0x46, 0x48, 
    0x4a, 0x4c, 0x4e, 0x50, 0x52, 0x2, 0x3, 0x5, 0x2, 0x7, 0x7, 0x20, 0x20, 
    0x34, 0x34, 0x2, 0x1f3, 0x2, 0x54, 0x3, 0x2, 0x2, 0x2, 0x4, 0x69, 0x3, 
    0x2, 0x2, 0x2, 0x6, 0x6c, 0x3, 0x2, 0x2, 0x2, 0x8, 0x74, 0x3, 0x2, 0x2, 
    0x2, 0xa, 0x7b, 0x3, 0x2, 0x2, 0x2, 0xc, 0x85, 0x3, 0x2, 0x2, 0x2, 0xe, 
    0x8c, 0x3, 0x2, 0x2, 0x2, 0x10, 0x90, 0x3, 0x2, 0x2, 0x2, 0x12, 0xa2, 
    0x3, 0x2, 0x2, 0x2, 0x14, 0xaf, 0x3, 0x2, 0x2, 0x2, 0x16, 0xb1, 0x3, 
    0x2, 0x2, 0x2, 0x18, 0xb8, 0x3, 0x2, 0x2, 0x2, 0x1a, 0xc3, 0x3, 0x2, 
    0x2, 0x2, 0x1c, 0xc5, 0x3, 0x2, 0x2, 0x2, 0x1e, 0xca, 0x3, 0x2, 0x2, 
    0x2, 0x20, 0xd2, 0x3, 0x2, 0x2, 0x2, 0x22, 0xdd, 0x3, 0x2, 0x2, 0x2, 
    0x24, 0xf8, 0x3, 0x2, 0x2, 0x2, 0x26, 0xfa, 0x3, 0x2, 0x2, 0x2, 0x28, 
    0x102, 0x3, 0x2, 0x2, 0x2, 0x2a, 0x107, 0x3, 0x2, 0x2, 0x2, 0x2c, 0x117, 
    0x3, 0x2, 0x2, 0x2, 0x2e, 0x144, 0x3, 0x2, 0x2, 0x2, 0x30, 0x14d, 0x3, 
    0x2, 0x2, 0x2, 0x32, 0x155, 0x3, 0x2, 0x2, 0x2, 0x34, 0x157, 0x3, 0x2, 
    0x2, 0x2, 0x36, 0x162, 0x3, 0x2, 0x2, 0x2, 0x38, 0x168, 0x3, 0x2, 0x2, 
    0x2, 0x3a, 0x172, 0x3, 0x2, 0x2, 0x2, 0x3c, 0x17d, 0x3, 0x2, 0x2, 0x2, 
    0x3e, 0x17f, 0x3, 0x2, 0x2, 0x2, 0x40, 0x182, 0x3, 0x2, 0x2, 0x2, 0x42, 
    0x18a, 0x3, 0x2, 0x2, 0x2, 0x44, 0x192, 0x3, 0x2, 0x2, 0x2, 0x46, 0x1ae, 
    0x3, 0x2, 0x2, 0x2, 0x48, 0x1b0, 0x3, 0x2, 0x2, 0x2, 0x4a, 0x1b6, 0x3, 
    0x2, 0x2, 0x2, 0x4c, 0x1b8, 0x3, 0x2, 0x2, 0x2, 0x4e, 0x1be, 0x3, 0x2, 
    0x2, 0x2, 0x50, 0x1c4, 0x3, 0x2, 0x2, 0x2, 0x52, 0x1cb, 0x3, 0x2, 0x2, 
    0x2, 0x54, 0x55, 0x5, 0x4, 0x3, 0x2, 0x55, 0x57, 0x5, 0x8, 0x5, 0x2, 
    0x56, 0x58, 0x5, 0x4c, 0x27, 0x2, 0x57, 0x56, 0x3, 0x2, 0x2, 0x2, 0x57, 
    0x58, 0x3, 0x2, 0x2, 0x2, 0x58, 0x59, 0x3, 0x2, 0x2, 0x2, 0x59, 0x5a, 
    0x5, 0x36, 0x1c, 0x2, 0x5a, 0x5c, 0x5, 0x3e, 0x20, 0x2, 0x5b, 0x5d, 
    0x5, 0x2a, 0x16, 0x2, 0x5c, 0x5b, 0x3, 0x2, 0x2, 0x2, 0x5c, 0x5d, 0x3, 
    0x2, 0x2, 0x2, 0x5d, 0x5f, 0x3, 0x2, 0x2, 0x2, 0x5e, 0x60, 0x5, 0x10, 
    0x9, 0x2, 0x5f, 0x5e, 0x3, 0x2, 0x2, 0x2, 0x5f, 0x60, 0x3, 0x2, 0x2, 
    0x2, 0x60, 0x62, 0x3, 0x2, 0x2, 0x2, 0x61, 0x63, 0x5, 0x26, 0x14, 0x2, 
    0x62, 0x61, 0x3, 0x2, 0x2, 0x2, 0x62, 0x63, 0x3, 0x2, 0x2, 0x2, 0x63, 
    0x65, 0x3, 0x2, 0x2, 0x2, 0x64, 0x66, 0x5, 0x48, 0x25, 0x2, 0x65, 0x64, 
    0x3, 0x2, 0x2, 0x2, 0x65, 0x66, 0x3, 0x2, 0x2, 0x2, 0x66, 0x67, 0x3, 
    0x2, 0x2, 0x2, 0x67, 0x68, 0x7, 0x2, 0x2, 0x3, 0x68, 0x3, 0x3, 0x2, 
    0x2, 0x2, 0x69, 0x6a, 0x7, 0x3, 0x2, 0x2, 0x6a, 0x6b, 0x5, 0x6, 0x4, 
    0x2, 0x6b, 0x5, 0x3, 0x2, 0x2, 0x2, 0x6c, 0x71, 0x7, 0x3d, 0x2, 0x2, 
    0x6d, 0x6e, 0x7, 0x4, 0x2, 0x2, 0x6e, 0x70, 0x7, 0x3d, 0x2, 0x2, 0x6f, 
    0x6d, 0x3, 0x2, 0x2, 0x2, 0x70, 0x73, 0x3, 0x2, 0x2, 0x2, 0x71, 0x6f, 
    0x3, 0x2, 0x2, 0x2, 0x71, 0x72, 0x3, 0x2, 0x2, 0x2, 0x72, 0x7, 0x3, 
    0x2, 0x2, 0x2, 0x73, 0x71, 0x3, 0x2, 0x2, 0x2, 0x74, 0x78, 0x7, 0x5, 
    0x2, 0x2, 0x75, 0x77, 0x5, 0xa, 0x6, 0x2, 0x76, 0x75, 0x3, 0x2, 0x2, 
    0x2, 0x77, 0x7a, 0x3, 0x2, 0x2, 0x2, 0x78, 0x76, 0x3, 0x2, 0x2, 0x2, 
    0x78, 0x79, 0x3, 0x2, 0x2, 0x2, 0x79, 0x9, 0x3, 0x2, 0x2, 0x2, 0x7a, 
    0x78, 0x3, 0x2, 0x2, 0x2, 0x7b, 0x7c, 0x5, 0xc, 0x7, 0x2, 0x7c, 0x80, 
    0x7, 0x3d, 0x2, 0x2, 0x7d, 0x7f, 0x5, 0xe, 0x8, 0x2, 0x7e, 0x7d, 0x3, 
    0x2, 0x2, 0x2, 0x7f, 0x82, 0x3, 0x2, 0x2, 0x2, 0x80, 0x7e, 0x3, 0x2, 
    0x2, 0x2, 0x80, 0x81, 0x3, 0x2, 0x2, 0x2, 0x81, 0x83, 0x3, 0x2, 0x2, 
    0x2, 0x82, 0x80, 0x3, 0x2, 0x2, 0x2, 0x83, 0x84, 0x7, 0x6, 0x2, 0x2, 
    0x84, 0xb, 0x3, 0x2, 0x2, 0x2, 0x85, 0x89, 0x5, 0x6, 0x4, 0x2, 0x86, 
    0x88, 0x7, 0x7, 0x2, 0x2, 0x87, 0x86, 0x3, 0x2, 0x2, 0x2, 0x88, 0x8b, 
    0x3, 0x2, 0x2, 0x2, 0x89, 0x87, 0x3, 0x2, 0x2, 0x2, 0x89, 0x8a, 0x3, 
    0x2, 0x2, 0x2, 0x8a, 0xd, 0x3, 0x2, 0x2, 0x2, 0x8b, 0x89, 0x3, 0x2, 
    0x2, 0x2, 0x8c, 0x8d, 0x7, 0x8, 0x2, 0x2, 0x8d, 0x8e, 0x7, 0x37, 0x2, 
    0x2, 0x8e, 0x8f, 0x7, 0x9, 0x2, 0x2, 0x8f, 0xf, 0x3, 0x2, 0x2, 0x2, 
    0x90, 0x96, 0x7, 0xa, 0x2, 0x2, 0x91, 0x92, 0x5, 0x12, 0xa, 0x2, 0x92, 
    0x93, 0x7, 0x6, 0x2, 0x2, 0x93, 0x95, 0x3, 0x2, 0x2, 0x2, 0x94, 0x91, 
    0x3, 0x2, 0x2, 0x2, 0x95, 0x98, 0x3, 0x2, 0x2, 0x2, 0x96, 0x94, 0x3, 
    0x2, 0x2, 0x2, 0x96, 0x97, 0x3, 0x2, 0x2, 0x2, 0x97, 0x11, 0x3, 0x2, 
    0x2, 0x2, 0x98, 0x96, 0x3, 0x2, 0x2, 0x2, 0x99, 0x9b, 0x8, 0xa, 0x1, 
    0x2, 0x9a, 0x9c, 0x7, 0xb, 0x2, 0x2, 0x9b, 0x9a, 0x3, 0x2, 0x2, 0x2, 
    0x9b, 0x9c, 0x3, 0x2, 0x2, 0x2, 0x9c, 0x9d, 0x3, 0x2, 0x2, 0x2, 0x9d, 
    0xa3, 0x5, 0x14, 0xb, 0x2, 0x9e, 0x9f, 0x5, 0x2c, 0x17, 0x2, 0x9f, 0xa0, 
    0x7, 0xe, 0x2, 0x2, 0xa0, 0xa1, 0x5, 0x12, 0xa, 0x3, 0xa1, 0xa3, 0x3, 
    0x2, 0x2, 0x2, 0xa2, 0x99, 0x3, 0x2, 0x2, 0x2, 0xa2, 0x9e, 0x3, 0x2, 
    0x2, 0x2, 0xa3, 0xac, 0x3, 0x2, 0x2, 0x2, 0xa4, 0xa5, 0xc, 0x5, 0x2, 
    0x2, 0xa5, 0xa6, 0x7, 0xc, 0x2, 0x2, 0xa6, 0xab, 0x5, 0x12, 0xa, 0x6, 
    0xa7, 0xa8, 0xc, 0x4, 0x2, 0x2, 0xa8, 0xa9, 0x7, 0xd, 0x2, 0x2, 0xa9, 
    0xab, 0x5, 0x12, 0xa, 0x5, 0xaa, 0xa4, 0x3, 0x2, 0x2, 0x2, 0xaa, 0xa7, 
    0x3, 0x2, 0x2, 0x2, 0xab, 0xae, 0x3, 0x2, 0x2, 0x2, 0xac, 0xaa, 0x3, 
    0x2, 0x2, 0x2, 0xac, 0xad, 0x3, 0x2, 0x2, 0x2, 0xad, 0x13, 0x3, 0x2, 
    0x2, 0x2, 0xae, 0xac, 0x3, 0x2, 0x2, 0x2, 0xaf, 0xb0, 0x5, 0x16, 0xc, 
    0x2, 0xb0, 0x15, 0x3, 0x2, 0x2, 0x2, 0xb1, 0xb2, 0x7, 0x3d, 0x2, 0x2, 
    0xb2, 0xb4, 0x7, 0x8, 0x2, 0x2, 0xb3, 0xb5, 0x5, 0x18, 0xd, 0x2, 0xb4, 
    0xb3, 0x3, 0x2, 0x2, 0x2, 0xb4, 0xb5, 0x3, 0x2, 0x2, 0x2, 0xb5, 0xb6, 
    0x3, 0x2, 0x2, 0x2, 0xb6, 0xb7, 0x7, 0x9, 0x2, 0x2, 0xb7, 0x17, 0x3, 
    0x2, 0x2, 0x2, 0xb8, 0xbd, 0x5, 0x1a, 0xe, 0x2, 0xb9, 0xba, 0x7, 0xc, 
    0x2, 0x2, 0xba, 0xbc, 0x5, 0x1a, 0xe, 0x2, 0xbb, 0xb9, 0x3, 0x2, 0x2, 
    0x2, 0xbc, 0xbf, 0x3, 0x2, 0x2, 0x2, 0xbd, 0xbb, 0x3, 0x2, 0x2, 0x2, 
    0xbd, 0xbe, 0x3, 0x2, 0x2, 0x2, 0xbe, 0x19, 0x3, 0x2, 0x2, 0x2, 0xbf, 
    0xbd, 0x3, 0x2, 0x2, 0x2, 0xc0, 0xc4, 0x5, 0x1c, 0xf, 0x2, 0xc1, 0xc4, 
    0x7, 0xf, 0x2, 0x2, 0xc2, 0xc4, 0x7, 0x10, 0x2, 0x2, 0xc3, 0xc0, 0x3, 
    0x2, 0x2, 0x2, 0xc3, 0xc1, 0x3, 0x2, 0x2, 0x2, 0xc3, 0xc2, 0x3, 0x2, 
    0x2, 0x2, 0xc4, 0x1b, 0x3, 0x2, 0x2, 0x2, 0xc5, 0xc6, 0x5, 0x1e, 0x10, 
    0x2, 0xc6, 0x1d, 0x3, 0x2, 0x2, 0x2, 0xc7, 0xcb, 0x5, 0x20, 0x11, 0x2, 
    0xc8, 0xcb, 0x5, 0x22, 0x12, 0x2, 0xc9, 0xcb, 0x5, 0x24, 0x13, 0x2, 
    0xca, 0xc7, 0x3, 0x2, 0x2, 0x2, 0xca, 0xc8, 0x3, 0x2, 0x2, 0x2, 0xca, 
    0xc9, 0x3, 0x2, 0x2, 0x2, 0xcb, 0x1f, 0x3, 0x2, 0x2, 0x2, 0xcc, 0xd3, 
    0x7, 0x37, 0x2, 0x2, 0xcd, 0xce, 0x7, 0x37, 0x2, 0x2, 0xce, 0xcf, 0x7, 
    0x11, 0x2, 0x2, 0xcf, 0xd3, 0x7, 0x37, 0x2, 0x2, 0xd0, 0xd3, 0x7, 0x3b, 
    0x2, 0x2, 0xd1, 0xd3, 0x7, 0x3c, 0x2, 0x2, 0xd2, 0xcc, 0x3, 0x2, 0x2, 
    0x2, 0xd2, 0xcd, 0x3, 0x2, 0x2, 0x2, 0xd2, 0xd0, 0x3, 0x2, 0x2, 0x2, 
    0xd2, 0xd1, 0x3, 0x2, 0x2, 0x2, 0xd3, 0x21, 0x3, 0x2, 0x2, 0x2, 0xd4, 
    0xde, 0x7, 0x3d, 0x2, 0x2, 0xd5, 0xd6, 0x7, 0x7, 0x2, 0x2, 0xd6, 0xde, 
    0x7, 0x3d, 0x2, 0x2, 0xd7, 0xd8, 0x7, 0x3d, 0x2, 0x2, 0xd8, 0xd9, 0x7, 
    0x12, 0x2, 0x2, 0xd9, 0xde, 0x7, 0x3d, 0x2, 0x2, 0xda, 0xdb, 0x7, 0x3d, 
    0x2, 0x2, 0xdb, 0xdc, 0x7, 0x13, 0x2, 0x2, 0xdc, 0xde, 0x7, 0x3d, 0x2, 
    0x2, 0xdd, 0xd4, 0x3, 0x2, 0x2, 0x2, 0xdd, 0xd5, 0x3, 0x2, 0x2, 0x2, 
    0xdd, 0xd7, 0x3, 0x2, 0x2, 0x2, 0xdd, 0xda, 0x3, 0x2, 0x2, 0x2, 0xde, 
    0x23, 0x3, 0x2, 0x2, 0x2, 0xdf, 0xe0, 0x7, 0x14, 0x2, 0x2, 0xe0, 0xe1, 
    0x7, 0x8, 0x2, 0x2, 0xe1, 0xe2, 0x5, 0x22, 0x12, 0x2, 0xe2, 0xe3, 0x7, 
    0xc, 0x2, 0x2, 0xe3, 0xe4, 0x5, 0xc, 0x7, 0x2, 0xe4, 0xe5, 0x7, 0x9, 
    0x2, 0x2, 0xe5, 0xf9, 0x3, 0x2, 0x2, 0x2, 0xe6, 0xe7, 0x7, 0x15, 0x2, 
    0x2, 0xe7, 0xe8, 0x7, 0x8, 0x2, 0x2, 0xe8, 0xe9, 0x7, 0x3d, 0x2, 0x2, 
    0xe9, 0xf9, 0x7, 0x9, 0x2, 0x2, 0xea, 0xeb, 0x7, 0x16, 0x2, 0x2, 0xeb, 
    0xec, 0x7, 0x8, 0x2, 0x2, 0xec, 0xed, 0x7, 0x3d, 0x2, 0x2, 0xed, 0xf9, 
    0x7, 0x9, 0x2, 0x2, 0xee, 0xef, 0x7, 0x17, 0x2, 0x2, 0xef, 0xf0, 0x7, 
    0x8, 0x2, 0x2, 0xf0, 0xf1, 0x5, 0x22, 0x12, 0x2, 0xf1, 0xf2, 0x7, 0x9, 
    0x2, 0x2, 0xf2, 0xf9, 0x3, 0x2, 0x2, 0x2, 0xf3, 0xf4, 0x7, 0x18, 0x2, 
    0x2, 0xf4, 0xf5, 0x7, 0x8, 0x2, 0x2, 0xf5, 0xf6, 0x5, 0x22, 0x12, 0x2, 
    0xf6, 0xf7, 0x7, 0x9, 0x2, 0x2, 0xf7, 0xf9, 0x3, 0x2, 0x2, 0x2, 0xf8, 
    0xdf, 0x3, 0x2, 0x2, 0x2, 0xf8, 0xe6, 0x3, 0x2, 0x2, 0x2, 0xf8, 0xea, 
    0x3, 0x2, 0x2, 0x2, 0xf8, 0xee, 0x3, 0x2, 0x2, 0x2, 0xf8, 0xf3, 0x3, 
    0x2, 0x2, 0x2, 0xf9, 0x25, 0x3, 0x2, 0x2, 0x2, 0xfa, 0xfe, 0x7, 0x19, 
    0x2, 0x2, 0xfb, 0xfc, 0x5, 0x28, 0x15, 0x2, 0xfc, 0xfd, 0x7, 0x6, 0x2, 
    0x2, 0xfd, 0xff, 0x3, 0x2, 0x2, 0x2, 0xfe, 0xfb, 0x3, 0x2, 0x2, 0x2, 
    0xff, 0x100, 0x3, 0x2, 0x2, 0x2, 0x100, 0xfe, 0x3, 0x2, 0x2, 0x2, 0x100, 
    0x101, 0x3, 0x2, 0x2, 0x2, 0x101, 0x27, 0x3, 0x2, 0x2, 0x2, 0x102, 0x105, 
    0x5, 0x16, 0xc, 0x2, 0x103, 0x104, 0x7, 0x1a, 0x2, 0x2, 0x104, 0x106, 
    0x7, 0x3d, 0x2, 0x2, 0x105, 0x103, 0x3, 0x2, 0x2, 0x2, 0x105, 0x106, 
    0x3, 0x2, 0x2, 0x2, 0x106, 0x29, 0x3, 0x2, 0x2, 0x2, 0x107, 0x10b, 0x7, 
    0x1b, 0x2, 0x2, 0x108, 0x109, 0x5, 0x2c, 0x17, 0x2, 0x109, 0x10a, 0x7, 
    0x6, 0x2, 0x2, 0x10a, 0x10c, 0x3, 0x2, 0x2, 0x2, 0x10b, 0x108, 0x3, 
    0x2, 0x2, 0x2, 0x10c, 0x10d, 0x3, 0x2, 0x2, 0x2, 0x10d, 0x10b, 0x3, 
    0x2, 0x2, 0x2, 0x10d, 0x10e, 0x3, 0x2, 0x2, 0x2, 0x10e, 0x2b, 0x3, 0x2, 
    0x2, 0x2, 0x10f, 0x118, 0x8, 0x17, 0x1, 0x2, 0x110, 0x111, 0x7, 0x1c, 
    0x2, 0x2, 0x111, 0x112, 0x5, 0x2c, 0x17, 0x2, 0x112, 0x113, 0x7, 0x1d, 
    0x2, 0x2, 0x113, 0x118, 0x3, 0x2, 0x2, 0x2, 0x114, 0x118, 0x5, 0x30, 
    0x19, 0x2, 0x115, 0x116, 0x7, 0xb, 0x2, 0x2, 0x116, 0x118, 0x5, 0x2c, 
    0x17, 0xb, 0x117, 0x10f, 0x3, 0x2, 0x2, 0x2, 0x117, 0x110, 0x3, 0x2, 
    0x2, 0x2, 0x117, 0x114, 0x3, 0x2, 0x2, 0x2, 0x117, 0x115, 0x3, 0x2, 
    0x2, 0x2, 0x118, 0x13d, 0x3, 0x2, 0x2, 0x2, 0x119, 0x11c, 0xc, 0xa, 
    0x2, 0x2, 0x11a, 0x11d, 0x7, 0x7, 0x2, 0x2, 0x11b, 0x11d, 0x7, 0x1e, 
    0x2, 0x2, 0x11c, 0x11a, 0x3, 0x2, 0x2, 0x2, 0x11c, 0x11b, 0x3, 0x2, 
    0x2, 0x2, 0x11d, 0x11e, 0x3, 0x2, 0x2, 0x2, 0x11e, 0x13c, 0x5, 0x2c, 
    0x17, 0xb, 0x11f, 0x120, 0xc, 0x9, 0x2, 0x2, 0x120, 0x121, 0x7, 0x1f, 
    0x2, 0x2, 0x121, 0x13c, 0x5, 0x2c, 0x17, 0xa, 0x122, 0x125, 0xc, 0x8, 
    0x2, 0x2, 0x123, 0x126, 0x7, 0x20, 0x2, 0x2, 0x124, 0x126, 0x7, 0x21, 
    0x2, 0x2, 0x125, 0x123, 0x3, 0x2, 0x2, 0x2, 0x125, 0x124, 0x3, 0x2, 
    0x2, 0x2, 0x126, 0x127, 0x3, 0x2, 0x2, 0x2, 0x127, 0x13c, 0x5, 0x2c, 
    0x17, 0x9, 0x128, 0x129, 0xc, 0x7, 0x2, 0x2, 0x129, 0x12a, 0x5, 0x2e, 
    0x18, 0x2, 0x12a, 0x12b, 0x5, 0x2c, 0x17, 0x8, 0x12b, 0x13c, 0x3, 0x2, 
    0x2, 0x2, 0x12c, 0x12f, 0xc, 0x6, 0x2, 0x2, 0x12d, 0x130, 0x7, 0x22, 
    0x2, 0x2, 0x12e, 0x130, 0x7, 0x23, 0x2, 0x2, 0x12f, 0x12d, 0x3, 0x2, 
    0x2, 0x2, 0x12f, 0x12e, 0x3, 0x2, 0x2, 0x2, 0x130, 0x131, 0x3, 0x2, 
    0x2, 0x2, 0x131, 0x13c, 0x5, 0x2c, 0x17, 0x7, 0x132, 0x133, 0xc, 0x5, 
    0x2, 0x2, 0x133, 0x134, 0x7, 0x24, 0x2, 0x2, 0x134, 0x13c, 0x5, 0x2c, 
    0x17, 0x6, 0x135, 0x136, 0xc, 0x4, 0x2, 0x2, 0x136, 0x137, 0x7, 0xd, 
    0x2, 0x2, 0x137, 0x13c, 0x5, 0x2c, 0x17, 0x5, 0x138, 0x139, 0xc, 0x3, 
    0x2, 0x2, 0x139, 0x13a, 0x7, 0xe, 0x2, 0x2, 0x13a, 0x13c, 0x5, 0x2c, 
    0x17, 0x3, 0x13b, 0x119, 0x3, 0x2, 0x2, 0x2, 0x13b, 0x11f, 0x3, 0x2, 
    0x2, 0x2, 0x13b, 0x122, 0x3, 0x2, 0x2, 0x2, 0x13b, 0x128, 0x3, 0x2, 
    0x2, 0x2, 0x13b, 0x12c, 0x3, 0x2, 0x2, 0x2, 0x13b, 0x132, 0x3, 0x2, 
    0x2, 0x2, 0x13b, 0x135, 0x3, 0x2, 0x2, 0x2, 0x13b, 0x138, 0x3, 0x2, 
    0x2, 0x2, 0x13c, 0x13f, 0x3, 0x2, 0x2, 0x2, 0x13d, 0x13b, 0x3, 0x2, 
    0x2, 0x2, 0x13d, 0x13e, 0x3, 0x2, 0x2, 0x2, 0x13e, 0x2d, 0x3, 0x2, 0x2, 
    0x2, 0x13f, 0x13d, 0x3, 0x2, 0x2, 0x2, 0x140, 0x145, 0x7, 0x25, 0x2, 
    0x2, 0x141, 0x145, 0x7, 0x26, 0x2, 0x2, 0x142, 0x145, 0x7, 0x27, 0x2, 
    0x2, 0x143, 0x145, 0x7, 0x28, 0x2, 0x2, 0x144, 0x140, 0x3, 0x2, 0x2, 
    0x2, 0x144, 0x141, 0x3, 0x2, 0x2, 0x2, 0x144, 0x142, 0x3, 0x2, 0x2, 
    0x2, 0x144, 0x143, 0x3, 0x2, 0x2, 0x2, 0x145, 0x2f, 0x3, 0x2, 0x2, 0x2, 
    0x146, 0x147, 0x5, 0x32, 0x1a, 0x2, 0x147, 0x148, 0x7, 0x29, 0x2, 0x2, 
    0x148, 0x149, 0x7, 0x2a, 0x2, 0x2, 0x149, 0x14a, 0x5, 0x34, 0x1b, 0x2, 
    0x14a, 0x14b, 0x7, 0x2b, 0x2, 0x2, 0x14b, 0x14e, 0x3, 0x2, 0x2, 0x2, 
    0x14c, 0x14e, 0x5, 0x1e, 0x10, 0x2, 0x14d, 0x146, 0x3, 0x2, 0x2, 0x2, 
    0x14d, 0x14c, 0x3, 0x2, 0x2, 0x2, 0x14e, 0x31, 0x3, 0x2, 0x2, 0x2, 0x14f, 
    0x150, 0x7, 0x2c, 0x2, 0x2, 0x150, 0x151, 0x7, 0x1c, 0x2, 0x2, 0x151, 
    0x152, 0x5, 0x1c, 0xf, 0x2, 0x152, 0x153, 0x7, 0x1d, 0x2, 0x2, 0x153, 
    0x156, 0x3, 0x2, 0x2, 0x2, 0x154, 0x156, 0x5, 0x1c, 0xf, 0x2, 0x155, 
    0x14f, 0x3, 0x2, 0x2, 0x2, 0x155, 0x154, 0x3, 0x2, 0x2, 0x2, 0x156, 
    0x33, 0x3, 0x2, 0x2, 0x2, 0x157, 0x15f, 0x5, 0x20, 0x11, 0x2, 0x158, 
    0x15b, 0x7, 0xc, 0x2, 0x2, 0x159, 0x15c, 0x5, 0x20, 0x11, 0x2, 0x15a, 
    0x15c, 0x7, 0x2d, 0x2, 0x2, 0x15b, 0x159, 0x3, 0x2, 0x2, 0x2, 0x15b, 
    0x15a, 0x3, 0x2, 0x2, 0x2, 0x15c, 0x15e, 0x3, 0x2, 0x2, 0x2, 0x15d, 
    0x158, 0x3, 0x2, 0x2, 0x2, 0x15e, 0x161, 0x3, 0x2, 0x2, 0x2, 0x15f, 
    0x15d, 0x3, 0x2, 0x2, 0x2, 0x15f, 0x160, 0x3, 0x2, 0x2, 0x2, 0x160, 
    0x35, 0x3, 0x2, 0x2, 0x2, 0x161, 0x15f, 0x3, 0x2, 0x2, 0x2, 0x162, 0x164, 
    0x7, 0x2e, 0x2, 0x2, 0x163, 0x165, 0x5, 0x38, 0x1d, 0x2, 0x164, 0x163, 
    0x3, 0x2, 0x2, 0x2, 0x165, 0x166, 0x3, 0x2, 0x2, 0x2, 0x166, 0x164, 
    0x3, 0x2, 0x2, 0x2, 0x166, 0x167, 0x3, 0x2, 0x2, 0x2, 0x167, 0x37, 0x3, 
    0x2, 0x2, 0x2, 0x168, 0x169, 0x7, 0x3d, 0x2, 0x2, 0x169, 0x16a, 0x7, 
    0x2f, 0x2, 0x2, 0x16a, 0x16b, 0x7, 0x3d, 0x2, 0x2, 0x16b, 0x16d, 0x7, 
    0x1c, 0x2, 0x2, 0x16c, 0x16e, 0x5, 0x3a, 0x1e, 0x2, 0x16d, 0x16c, 0x3, 
    0x2, 0x2, 0x2, 0x16d, 0x16e, 0x3, 0x2, 0x2, 0x2, 0x16e, 0x16f, 0x3, 
    0x2, 0x2, 0x2, 0x16f, 0x170, 0x7, 0x1d, 0x2, 0x2, 0x170, 0x171, 0x7, 
    0x6, 0x2, 0x2, 0x171, 0x39, 0x3, 0x2, 0x2, 0x2, 0x172, 0x177, 0x5, 0x3c, 
    0x1f, 0x2, 0x173, 0x174, 0x7, 0xc, 0x2, 0x2, 0x174, 0x176, 0x5, 0x3c, 
    0x1f, 0x2, 0x175, 0x173, 0x3, 0x2, 0x2, 0x2, 0x176, 0x179, 0x3, 0x2, 
    0x2, 0x2, 0x177, 0x175, 0x3, 0x2, 0x2, 0x2, 0x177, 0x178, 0x3, 0x2, 
    0x2, 0x2, 0x178, 0x3b, 0x3, 0x2, 0x2, 0x2, 0x179, 0x177, 0x3, 0x2, 0x2, 
    0x2, 0x17a, 0x17e, 0x5, 0x22, 0x12, 0x2, 0x17b, 0x17e, 0x7, 0xf, 0x2, 
    0x2, 0x17c, 0x17e, 0x7, 0x10, 0x2, 0x2, 0x17d, 0x17a, 0x3, 0x2, 0x2, 
    0x2, 0x17d, 0x17b, 0x3, 0x2, 0x2, 0x2, 0x17d, 0x17c, 0x3, 0x2, 0x2, 
    0x2, 0x17e, 0x3d, 0x3, 0x2, 0x2, 0x2, 0x17f, 0x180, 0x7, 0x30, 0x2, 
    0x2, 0x180, 0x181, 0x5, 0x40, 0x21, 0x2, 0x181, 0x3f, 0x3, 0x2, 0x2, 
    0x2, 0x182, 0x187, 0x5, 0x42, 0x22, 0x2, 0x183, 0x184, 0x7, 0xc, 0x2, 
    0x2, 0x184, 0x186, 0x5, 0x42, 0x22, 0x2, 0x185, 0x183, 0x3, 0x2, 0x2, 
    0x2, 0x186, 0x189, 0x3, 0x2, 0x2, 0x2, 0x187, 0x185, 0x3, 0x2, 0x2, 
    0x2, 0x187, 0x188, 0x3, 0x2, 0x2, 0x2, 0x188, 0x41, 0x3, 0x2, 0x2, 0x2, 
    0x189, 0x187, 0x3, 0x2, 0x2, 0x2, 0x18a, 0x18f, 0x5, 0x44, 0x23, 0x2, 
    0x18b, 0x18c, 0x7, 0x31, 0x2, 0x2, 0x18c, 0x18e, 0x5, 0x44, 0x23, 0x2, 
    0x18d, 0x18b, 0x3, 0x2, 0x2, 0x2, 0x18e, 0x191, 0x3, 0x2, 0x2, 0x2, 
    0x18f, 0x18d, 0x3, 0x2, 0x2, 0x2, 0x18f, 0x190, 0x3, 0x2, 0x2, 0x2, 
    0x190, 0x43, 0x3, 0x2, 0x2, 0x2, 0x191, 0x18f, 0x3, 0x2, 0x2, 0x2, 0x192, 
    0x1a2, 0x5, 0x46, 0x24, 0x2, 0x193, 0x194, 0x7, 0x32, 0x2, 0x2, 0x194, 
    0x196, 0x5, 0x46, 0x24, 0x2, 0x195, 0x193, 0x3, 0x2, 0x2, 0x2, 0x196, 
    0x197, 0x3, 0x2, 0x2, 0x2, 0x197, 0x195, 0x3, 0x2, 0x2, 0x2, 0x197, 
    0x198, 0x3, 0x2, 0x2, 0x2, 0x198, 0x1a0, 0x3, 0x2, 0x2, 0x2, 0x199, 
    0x19b, 0x7, 0x37, 0x2, 0x2, 0x19a, 0x199, 0x3, 0x2, 0x2, 0x2, 0x19a, 
    0x19b, 0x3, 0x2, 0x2, 0x2, 0x19b, 0x19c, 0x3, 0x2, 0x2, 0x2, 0x19c, 
    0x19e, 0x7, 0x33, 0x2, 0x2, 0x19d, 0x19f, 0x7, 0x37, 0x2, 0x2, 0x19e, 
    0x19d, 0x3, 0x2, 0x2, 0x2, 0x19e, 0x19f, 0x3, 0x2, 0x2, 0x2, 0x19f, 
    0x1a1, 0x3, 0x2, 0x2, 0x2, 0x1a0, 0x19a, 0x3, 0x2, 0x2, 0x2, 0x1a0, 
    0x1a1, 0x3, 0x2, 0x2, 0x2, 0x1a1, 0x1a3, 0x3, 0x2, 0x2, 0x2, 0x1a2, 
    0x195, 0x3, 0x2, 0x2, 0x2, 0x1a2, 0x1a3, 0x3, 0x2, 0x2, 0x2, 0x1a3, 
    0x45, 0x3, 0x2, 0x2, 0x2, 0x1a4, 0x1a6, 0x7, 0x3d, 0x2, 0x2, 0x1a5, 
    0x1a7, 0x9, 0x2, 0x2, 0x2, 0x1a6, 0x1a5, 0x3, 0x2, 0x2, 0x2, 0x1a6, 
    0x1a7, 0x3, 0x2, 0x2, 0x2, 0x1a7, 0x1af, 0x3, 0x2, 0x2, 0x2, 0x1a8, 
    0x1a9, 0x7, 0x1c, 0x2, 0x2, 0x1a9, 0x1aa, 0x5, 0x40, 0x21, 0x2, 0x1aa, 
    0x1ac, 0x7, 0x1d, 0x2, 0x2, 0x1ab, 0x1ad, 0x9, 0x2, 0x2, 0x2, 0x1ac, 
    0x1ab, 0x3, 0x2, 0x2, 0x2, 0x1ac, 0x1ad, 0x3, 0x2, 0x2, 0x2, 0x1ad, 
    0x1af, 0x3, 0x2, 0x2, 0x2, 0x1ae, 0x1a4, 0x3, 0x2, 0x2, 0x2, 0x1ae, 
    0x1a8, 0x3, 0x2, 0x2, 0x2, 0x1af, 0x47, 0x3, 0x2, 0x2, 0x2, 0x1b0, 0x1b2, 
    0x7, 0x35, 0x2, 0x2, 0x1b1, 0x1b3, 0x5, 0x4a, 0x26, 0x2, 0x1b2, 0x1b1, 
    0x3, 0x2, 0x2, 0x2, 0x1b3, 0x1b4, 0x3, 0x2, 0x2, 0x2, 0x1b4, 0x1b2, 
    0x3, 0x2, 0x2, 0x2, 0x1b4, 0x1b5, 0x3, 0x2, 0x2, 0x2, 0x1b5, 0x49, 0x3, 
    0x2, 0x2, 0x2, 0x1b6, 0x1b7, 0x5, 0x28, 0x15, 0x2, 0x1b7, 0x4b, 0x3, 
    0x2, 0x2, 0x2, 0x1b8, 0x1ba, 0x7, 0x36, 0x2, 0x2, 0x1b9, 0x1bb, 0x5, 
    0x4e, 0x28, 0x2, 0x1ba, 0x1b9, 0x3, 0x2, 0x2, 0x2, 0x1bb, 0x1bc, 0x3, 
    0x2, 0x2, 0x2, 0x1bc, 0x1ba, 0x3, 0x2, 0x2, 0x2, 0x1bc, 0x1bd, 0x3, 
    0x2, 0x2, 0x2, 0x1bd, 0x4d, 0x3, 0x2, 0x2, 0x2, 0x1be, 0x1bf, 0x5, 0x50, 
    0x29, 0x2, 0x1bf, 0x1c0, 0x7, 0xe, 0x2, 0x2, 0x1c0, 0x1c1, 0x7, 0x3d, 
    0x2, 0x2, 0x1c1, 0x1c2, 0x3, 0x2, 0x2, 0x2, 0x1c2, 0x1c3, 0x7, 0x6, 
    0x2, 0x2, 0x1c3, 0x4f, 0x3, 0x2, 0x2, 0x2, 0x1c4, 0x1c5, 0x5, 0x6, 0x4, 
    0x2, 0x1c5, 0x1c7, 0x7, 0x1c, 0x2, 0x2, 0x1c6, 0x1c8, 0x5, 0x52, 0x2a, 
    0x2, 0x1c7, 0x1c6, 0x3, 0x2, 0x2, 0x2, 0x1c7, 0x1c8, 0x3, 0x2, 0x2, 
    0x2, 0x1c8, 0x1c9, 0x3, 0x2, 0x2, 0x2, 0x1c9, 0x1ca, 0x7, 0x1d, 0x2, 
    0x2, 0x1ca, 0x51, 0x3, 0x2, 0x2, 0x2, 0x1cb, 0x1d0, 0x5, 0xc, 0x7, 0x2, 
    0x1cc, 0x1cd, 0x7, 0xc, 0x2, 0x2, 0x1cd, 0x1cf, 0x5, 0xc, 0x7, 0x2, 
    0x1ce, 0x1cc, 0x3, 0x2, 0x2, 0x2, 0x1cf, 0x1d2, 0x3, 0x2, 0x2, 0x2, 
    0x1d0, 0x1ce, 0x3, 0x2, 0x2, 0x2, 0x1d0, 0x1d1, 0x3, 0x2, 0x2, 0x2, 
    0x1d1, 0x53, 0x3, 0x2, 0x2, 0x2, 0x1d2, 0x1d0, 0x3, 0x2, 0x2, 0x2, 0x37, 
    0x57, 0x5c, 0x5f, 0x62, 0x65, 0x71, 0x78, 0x80, 0x89, 0x96, 0x9b, 0xa2, 
    0xaa, 0xac, 0xb4, 0xbd, 0xc3, 0xca, 0xd2, 0xdd, 0xf8, 0x100, 0x105, 
    0x10d, 0x117, 0x11c, 0x125, 0x12f, 0x13b, 0x13d, 0x144, 0x14d, 0x155, 
    0x15b, 0x15f, 0x166, 0x16d, 0x177, 0x17d, 0x187, 0x18f, 0x197, 0x19a, 
    0x19e, 0x1a0, 0x1a2, 0x1a6, 0x1ac, 0x1ae, 0x1b4, 0x1bc, 0x1c7, 0x1d0, 
  };

  atn::ATNDeserializer deserializer;
  _atn = deserializer.deserialize(_serializedATN);

  size_t count = _atn.getNumberOfDecisions();
  _decisionToDFA.reserve(count);
  for (size_t i = 0; i < count; i++) { 
    _decisionToDFA.emplace_back(_atn.getDecisionState(i), i);
  }
}

CrySLParser::Initializer CrySLParser::_init;
