
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
    setState(86);
    spec();
    setState(87);
    objects();
    setState(89);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == CrySLParser::T__62) {
      setState(88);
      forbidden();
    }
    setState(91);
    events();
    setState(92);
    order();
    setState(94);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == CrySLParser::T__34) {
      setState(93);
      constraints();
    }
    setState(97);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == CrySLParser::T__17) {
      setState(96);
      requiresBlock();
    }
    setState(100);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == CrySLParser::T__32) {
      setState(99);
      ensures();
    }
    setState(103);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == CrySLParser::T__61) {
      setState(102);
      negates();
    }
    setState(105);
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
    setState(107);
    match(CrySLParser::T__0);
    setState(108);
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
    setState(110);
    match(CrySLParser::Ident);
    setState(115);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == CrySLParser::T__1) {
      setState(111);
      match(CrySLParser::T__1);
      setState(112);
      match(CrySLParser::Ident);
      setState(117);
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
    setState(118);
    match(CrySLParser::T__2);
    setState(122);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & ((1ULL << CrySLParser::T__3)
      | (1ULL << CrySLParser::T__5)
      | (1ULL << CrySLParser::T__6)
      | (1ULL << CrySLParser::T__7)
      | (1ULL << CrySLParser::T__8)
      | (1ULL << CrySLParser::T__9)
      | (1ULL << CrySLParser::T__10)
      | (1ULL << CrySLParser::T__11)
      | (1ULL << CrySLParser::T__12)
      | (1ULL << CrySLParser::T__13))) != 0) || _la == CrySLParser::Ident) {
      setState(119);
      objectDecl();
      setState(124);
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
    setState(126);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == CrySLParser::T__3) {
      setState(125);
      dynamic_cast<ObjectDeclContext *>(_localctx)->constModifier = match(CrySLParser::T__3);
    }
    setState(128);
    typeName();
    setState(129);
    match(CrySLParser::Ident);
    setState(133);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == CrySLParser::T__15) {
      setState(130);
      array();
      setState(135);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(136);
    match(CrySLParser::T__4);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- PrimitiveTypeNameContext ------------------------------------------------------------------

CrySLParser::PrimitiveTypeNameContext::PrimitiveTypeNameContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t CrySLParser::PrimitiveTypeNameContext::getRuleIndex() const {
  return CrySLParser::RulePrimitiveTypeName;
}


CrySLParser::PrimitiveTypeNameContext* CrySLParser::primitiveTypeName() {
  PrimitiveTypeNameContext *_localctx = _tracker.createInstance<PrimitiveTypeNameContext>(_ctx, getState());
  enterRule(_localctx, 10, CrySLParser::RulePrimitiveTypeName);
  size_t _la = 0;

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    setState(156);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 12, _ctx)) {
    case 1: {
      enterOuterAlt(_localctx, 1);
      setState(138);
      dynamic_cast<PrimitiveTypeNameContext *>(_localctx)->booleanType = match(CrySLParser::T__5);
      break;
    }

    case 2: {
      enterOuterAlt(_localctx, 2);
      setState(140);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == CrySLParser::T__6) {
        setState(139);
        dynamic_cast<PrimitiveTypeNameContext *>(_localctx)->unsignedInt = match(CrySLParser::T__6);
      }
      setState(148);
      _errHandler->sync(this);
      switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 10, _ctx)) {
      case 1: {
        setState(142);
        dynamic_cast<PrimitiveTypeNameContext *>(_localctx)->charTy = match(CrySLParser::T__7);
        break;
      }

      case 2: {
        setState(143);
        dynamic_cast<PrimitiveTypeNameContext *>(_localctx)->shortTy = match(CrySLParser::T__8);
        break;
      }

      case 3: {
        setState(144);
        dynamic_cast<PrimitiveTypeNameContext *>(_localctx)->intTy = match(CrySLParser::T__9);
        break;
      }

      case 4: {
        setState(145);
        dynamic_cast<PrimitiveTypeNameContext *>(_localctx)->longTy = match(CrySLParser::T__10);
        break;
      }

      case 5: {
        setState(146);
        dynamic_cast<PrimitiveTypeNameContext *>(_localctx)->longlongTy = match(CrySLParser::T__10);
        setState(147);
        match(CrySLParser::T__10);
        break;
      }

      }
      break;
    }

    case 3: {
      enterOuterAlt(_localctx, 3);
      setState(150);
      dynamic_cast<PrimitiveTypeNameContext *>(_localctx)->floatingPoint = match(CrySLParser::T__11);
      break;
    }

    case 4: {
      enterOuterAlt(_localctx, 4);
      setState(152);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == CrySLParser::T__10) {
        setState(151);
        dynamic_cast<PrimitiveTypeNameContext *>(_localctx)->longDouble = match(CrySLParser::T__10);
      }
      setState(154);
      dynamic_cast<PrimitiveTypeNameContext *>(_localctx)->doubleFloat = match(CrySLParser::T__12);
      break;
    }

    case 5: {
      enterOuterAlt(_localctx, 5);
      setState(155);
      dynamic_cast<PrimitiveTypeNameContext *>(_localctx)->sizeType = match(CrySLParser::T__13);
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

//----------------- TypeNameContext ------------------------------------------------------------------

CrySLParser::TypeNameContext::TypeNameContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

CrySLParser::QualifiedNameContext* CrySLParser::TypeNameContext::qualifiedName() {
  return getRuleContext<CrySLParser::QualifiedNameContext>(0);
}

CrySLParser::PrimitiveTypeNameContext* CrySLParser::TypeNameContext::primitiveTypeName() {
  return getRuleContext<CrySLParser::PrimitiveTypeNameContext>(0);
}

std::vector<CrySLParser::PtrContext *> CrySLParser::TypeNameContext::ptr() {
  return getRuleContexts<CrySLParser::PtrContext>();
}

CrySLParser::PtrContext* CrySLParser::TypeNameContext::ptr(size_t i) {
  return getRuleContext<CrySLParser::PtrContext>(i);
}


size_t CrySLParser::TypeNameContext::getRuleIndex() const {
  return CrySLParser::RuleTypeName;
}


CrySLParser::TypeNameContext* CrySLParser::typeName() {
  TypeNameContext *_localctx = _tracker.createInstance<TypeNameContext>(_ctx, getState());
  enterRule(_localctx, 12, CrySLParser::RuleTypeName);
  size_t _la = 0;

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(160);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case CrySLParser::Ident: {
        setState(158);
        qualifiedName();
        break;
      }

      case CrySLParser::T__5:
      case CrySLParser::T__6:
      case CrySLParser::T__7:
      case CrySLParser::T__8:
      case CrySLParser::T__9:
      case CrySLParser::T__10:
      case CrySLParser::T__11:
      case CrySLParser::T__12:
      case CrySLParser::T__13: {
        setState(159);
        primitiveTypeName();
        break;
      }

    default:
      throw NoViableAltException(this);
    }
    setState(165);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == CrySLParser::T__14) {
      setState(162);
      dynamic_cast<TypeNameContext *>(_localctx)->pointer = ptr();
      setState(167);
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

//----------------- PtrContext ------------------------------------------------------------------

CrySLParser::PtrContext::PtrContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t CrySLParser::PtrContext::getRuleIndex() const {
  return CrySLParser::RulePtr;
}


CrySLParser::PtrContext* CrySLParser::ptr() {
  PtrContext *_localctx = _tracker.createInstance<PtrContext>(_ctx, getState());
  enterRule(_localctx, 14, CrySLParser::RulePtr);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(168);
    match(CrySLParser::T__14);
   
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
  enterRule(_localctx, 16, CrySLParser::RuleArray);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(170);
    match(CrySLParser::T__15);
    setState(171);
    match(CrySLParser::Int);
    setState(172);
    match(CrySLParser::T__16);
   
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
  enterRule(_localctx, 18, CrySLParser::RuleRequiresBlock);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    size_t alt;
    enterOuterAlt(_localctx, 1);
    setState(174);
    match(CrySLParser::T__17);
    setState(180);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 15, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        setState(175);
        reqPred(0);
        setState(176);
        match(CrySLParser::T__4); 
      }
      setState(182);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 15, _ctx);
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
  size_t startState = 20;
  enterRecursionRule(_localctx, 20, CrySLParser::RuleReqPred, precedence);

    size_t _la = 0;

  auto onExit = finally([=] {
    unrollRecursionContexts(parentContext);
  });
  try {
    size_t alt;
    enterOuterAlt(_localctx, 1);
    setState(192);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 17, _ctx)) {
    case 1: {
      setState(185);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == CrySLParser::T__18) {
        setState(184);
        dynamic_cast<ReqPredContext *>(_localctx)->neg = match(CrySLParser::T__18);
      }
      setState(187);
      reqPredLit();
      break;
    }

    case 2: {
      setState(188);
      constr(0);
      setState(189);
      dynamic_cast<ReqPredContext *>(_localctx)->implication = match(CrySLParser::T__21);
      setState(190);
      reqPred(1);
      break;
    }

    }
    _ctx->stop = _input->LT(-1);
    setState(202);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 19, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        if (!_parseListeners.empty())
          triggerExitRuleEvent();
        previousContext = _localctx;
        setState(200);
        _errHandler->sync(this);
        switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 18, _ctx)) {
        case 1: {
          _localctx = _tracker.createInstance<ReqPredContext>(parentContext, parentState);
          pushNewRecursionContext(_localctx, startState, RuleReqPred);
          setState(194);

          if (!(precpred(_ctx, 3))) throw FailedPredicateException(this, "precpred(_ctx, 3)");
          setState(195);
          dynamic_cast<ReqPredContext *>(_localctx)->seq = match(CrySLParser::T__19);
          setState(196);
          reqPred(4);
          break;
        }

        case 2: {
          _localctx = _tracker.createInstance<ReqPredContext>(parentContext, parentState);
          pushNewRecursionContext(_localctx, startState, RuleReqPred);
          setState(197);

          if (!(precpred(_ctx, 2))) throw FailedPredicateException(this, "precpred(_ctx, 2)");
          setState(198);
          dynamic_cast<ReqPredContext *>(_localctx)->alt = match(CrySLParser::T__20);
          setState(199);
          reqPred(3);
          break;
        }

        } 
      }
      setState(204);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 19, _ctx);
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
  enterRule(_localctx, 22, CrySLParser::RuleReqPredLit);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(205);
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
  enterRule(_localctx, 24, CrySLParser::RulePred);
  size_t _la = 0;

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(207);
    dynamic_cast<PredContext *>(_localctx)->name = match(CrySLParser::Ident);
    setState(208);
    match(CrySLParser::T__15);
    setState(210);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (((((_la - 15) & ~ 0x3fULL) == 0) &&
      ((1ULL << (_la - 15)) & ((1ULL << (CrySLParser::T__14 - 15))
      | (1ULL << (CrySLParser::T__22 - 15))
      | (1ULL << (CrySLParser::T__23 - 15))
      | (1ULL << (CrySLParser::T__27 - 15))
      | (1ULL << (CrySLParser::T__28 - 15))
      | (1ULL << (CrySLParser::T__29 - 15))
      | (1ULL << (CrySLParser::T__30 - 15))
      | (1ULL << (CrySLParser::T__31 - 15))
      | (1ULL << (CrySLParser::Int - 15))
      | (1ULL << (CrySLParser::Bool - 15))
      | (1ULL << (CrySLParser::String - 15))
      | (1ULL << (CrySLParser::Ident - 15)))) != 0)) {
      setState(209);
      dynamic_cast<PredContext *>(_localctx)->paramList = suParList();
    }
    setState(212);
    match(CrySLParser::T__16);
   
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
  enterRule(_localctx, 26, CrySLParser::RuleSuParList);
  size_t _la = 0;

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(214);
    suPar();
    setState(219);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == CrySLParser::T__19) {
      setState(215);
      match(CrySLParser::T__19);
      setState(216);
      suPar();
      setState(221);
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
  enterRule(_localctx, 28, CrySLParser::RuleSuPar);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    setState(225);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case CrySLParser::T__14:
      case CrySLParser::T__27:
      case CrySLParser::T__28:
      case CrySLParser::T__29:
      case CrySLParser::T__30:
      case CrySLParser::T__31:
      case CrySLParser::Int:
      case CrySLParser::Bool:
      case CrySLParser::String:
      case CrySLParser::Ident: {
        enterOuterAlt(_localctx, 1);
        setState(222);
        dynamic_cast<SuParContext *>(_localctx)->value = consPred();
        break;
      }

      case CrySLParser::T__22: {
        enterOuterAlt(_localctx, 2);
        setState(223);
        dynamic_cast<SuParContext *>(_localctx)->thisptr = match(CrySLParser::T__22);
        break;
      }

      case CrySLParser::T__23: {
        enterOuterAlt(_localctx, 3);
        setState(224);
        dynamic_cast<SuParContext *>(_localctx)->wildcard = match(CrySLParser::T__23);
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
  enterRule(_localctx, 30, CrySLParser::RuleConsPred);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(227);
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
  enterRule(_localctx, 32, CrySLParser::RuleLiteralExpr);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    setState(232);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case CrySLParser::Int:
      case CrySLParser::Bool:
      case CrySLParser::String: {
        enterOuterAlt(_localctx, 1);
        setState(229);
        literal();
        break;
      }

      case CrySLParser::T__14:
      case CrySLParser::Ident: {
        enterOuterAlt(_localctx, 2);
        setState(230);
        memberAccess();
        break;
      }

      case CrySLParser::T__27:
      case CrySLParser::T__28:
      case CrySLParser::T__29:
      case CrySLParser::T__30:
      case CrySLParser::T__31: {
        enterOuterAlt(_localctx, 3);
        setState(231);
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
  enterRule(_localctx, 34, CrySLParser::RuleLiteral);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    setState(240);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 24, _ctx)) {
    case 1: {
      enterOuterAlt(_localctx, 1);
      setState(234);
      match(CrySLParser::Int);
      break;
    }

    case 2: {
      enterOuterAlt(_localctx, 2);
      setState(235);
      dynamic_cast<LiteralContext *>(_localctx)->base = match(CrySLParser::Int);
      setState(236);
      dynamic_cast<LiteralContext *>(_localctx)->pow = match(CrySLParser::T__24);
      setState(237);
      dynamic_cast<LiteralContext *>(_localctx)->exp = match(CrySLParser::Int);
      break;
    }

    case 3: {
      enterOuterAlt(_localctx, 3);
      setState(238);
      match(CrySLParser::Bool);
      break;
    }

    case 4: {
      enterOuterAlt(_localctx, 4);
      setState(239);
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
  enterRule(_localctx, 36, CrySLParser::RuleMemberAccess);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    setState(251);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 25, _ctx)) {
    case 1: {
      enterOuterAlt(_localctx, 1);
      setState(242);
      match(CrySLParser::Ident);
      break;
    }

    case 2: {
      enterOuterAlt(_localctx, 2);
      setState(243);
      dynamic_cast<MemberAccessContext *>(_localctx)->deref = match(CrySLParser::T__14);
      setState(244);
      match(CrySLParser::Ident);
      break;
    }

    case 3: {
      enterOuterAlt(_localctx, 3);
      setState(245);
      match(CrySLParser::Ident);
      setState(246);
      dynamic_cast<MemberAccessContext *>(_localctx)->dot = match(CrySLParser::T__25);
      setState(247);
      match(CrySLParser::Ident);
      break;
    }

    case 4: {
      enterOuterAlt(_localctx, 4);
      setState(248);
      match(CrySLParser::Ident);
      setState(249);
      dynamic_cast<MemberAccessContext *>(_localctx)->arrow = match(CrySLParser::T__26);
      setState(250);
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
  enterRule(_localctx, 38, CrySLParser::RulePreDefinedPredicate);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    setState(278);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case CrySLParser::T__27: {
        enterOuterAlt(_localctx, 1);
        setState(253);
        dynamic_cast<PreDefinedPredicateContext *>(_localctx)->name = match(CrySLParser::T__27);
        setState(254);
        match(CrySLParser::T__15);
        setState(255);
        dynamic_cast<PreDefinedPredicateContext *>(_localctx)->obj = memberAccess();
        setState(256);
        match(CrySLParser::T__19);
        setState(257);
        dynamic_cast<PreDefinedPredicateContext *>(_localctx)->type = typeName();
        setState(258);
        match(CrySLParser::T__16);
        break;
      }

      case CrySLParser::T__28: {
        enterOuterAlt(_localctx, 2);
        setState(260);
        dynamic_cast<PreDefinedPredicateContext *>(_localctx)->name = match(CrySLParser::T__28);
        setState(261);
        match(CrySLParser::T__15);
        setState(262);
        dynamic_cast<PreDefinedPredicateContext *>(_localctx)->evt = match(CrySLParser::Ident);
        setState(263);
        match(CrySLParser::T__16);
        break;
      }

      case CrySLParser::T__29: {
        enterOuterAlt(_localctx, 3);
        setState(264);
        dynamic_cast<PreDefinedPredicateContext *>(_localctx)->name = match(CrySLParser::T__29);
        setState(265);
        match(CrySLParser::T__15);
        setState(266);
        dynamic_cast<PreDefinedPredicateContext *>(_localctx)->evt = match(CrySLParser::Ident);
        setState(267);
        match(CrySLParser::T__16);
        break;
      }

      case CrySLParser::T__30: {
        enterOuterAlt(_localctx, 4);
        setState(268);
        dynamic_cast<PreDefinedPredicateContext *>(_localctx)->name = match(CrySLParser::T__30);
        setState(269);
        match(CrySLParser::T__15);
        setState(270);
        dynamic_cast<PreDefinedPredicateContext *>(_localctx)->obj = memberAccess();
        setState(271);
        match(CrySLParser::T__16);
        break;
      }

      case CrySLParser::T__31: {
        enterOuterAlt(_localctx, 5);
        setState(273);
        dynamic_cast<PreDefinedPredicateContext *>(_localctx)->name = match(CrySLParser::T__31);
        setState(274);
        match(CrySLParser::T__15);
        setState(275);
        dynamic_cast<PreDefinedPredicateContext *>(_localctx)->obj = memberAccess();
        setState(276);
        match(CrySLParser::T__16);
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
  enterRule(_localctx, 40, CrySLParser::RuleEnsures);
  size_t _la = 0;

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(280);
    match(CrySLParser::T__32);
    setState(284); 
    _errHandler->sync(this);
    _la = _input->LA(1);
    do {
      setState(281);
      ensPred();
      setState(282);
      match(CrySLParser::T__4);
      setState(286); 
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
  enterRule(_localctx, 42, CrySLParser::RuleEnsPred);
  size_t _la = 0;

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(288);
    pred();
    setState(291);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == CrySLParser::T__33) {
      setState(289);
      match(CrySLParser::T__33);
      setState(290);
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
  enterRule(_localctx, 44, CrySLParser::RuleConstraints);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    size_t alt;
    enterOuterAlt(_localctx, 1);
    setState(293);
    match(CrySLParser::T__34);
    setState(297); 
    _errHandler->sync(this);
    alt = 1;
    do {
      switch (alt) {
        case 1: {
              setState(294);
              constr(0);
              setState(295);
              match(CrySLParser::T__4);
              break;
            }

      default:
        throw NoViableAltException(this);
      }
      setState(299); 
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 29, _ctx);
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
  size_t startState = 46;
  enterRecursionRule(_localctx, 46, CrySLParser::RuleConstr, precedence);

    

  auto onExit = finally([=] {
    unrollRecursionContexts(parentContext);
  });
  try {
    size_t alt;
    enterOuterAlt(_localctx, 1);
    setState(309);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 30, _ctx)) {
    case 1: {
      break;
    }

    case 2: {
      setState(302);
      match(CrySLParser::T__35);
      setState(303);
      constr(0);
      setState(304);
      match(CrySLParser::T__36);
      break;
    }

    case 3: {
      setState(306);
      cons();
      break;
    }

    case 4: {
      setState(307);
      dynamic_cast<ConstrContext *>(_localctx)->lnot = match(CrySLParser::T__18);
      setState(308);
      constr(9);
      break;
    }

    }
    _ctx->stop = _input->LT(-1);
    setState(347);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 35, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        if (!_parseListeners.empty())
          triggerExitRuleEvent();
        previousContext = _localctx;
        setState(345);
        _errHandler->sync(this);
        switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 34, _ctx)) {
        case 1: {
          _localctx = _tracker.createInstance<ConstrContext>(parentContext, parentState);
          pushNewRecursionContext(_localctx, startState, RuleConstr);
          setState(311);

          if (!(precpred(_ctx, 8))) throw FailedPredicateException(this, "precpred(_ctx, 8)");
          setState(314);
          _errHandler->sync(this);
          switch (_input->LA(1)) {
            case CrySLParser::T__14: {
              setState(312);
              dynamic_cast<ConstrContext *>(_localctx)->mul = match(CrySLParser::T__14);
              break;
            }

            case CrySLParser::T__37: {
              setState(313);
              dynamic_cast<ConstrContext *>(_localctx)->div = match(CrySLParser::T__37);
              break;
            }

          default:
            throw NoViableAltException(this);
          }
          setState(316);
          constr(9);
          break;
        }

        case 2: {
          _localctx = _tracker.createInstance<ConstrContext>(parentContext, parentState);
          pushNewRecursionContext(_localctx, startState, RuleConstr);
          setState(317);

          if (!(precpred(_ctx, 7))) throw FailedPredicateException(this, "precpred(_ctx, 7)");
          setState(318);
          dynamic_cast<ConstrContext *>(_localctx)->mod = match(CrySLParser::T__38);
          setState(319);
          constr(8);
          break;
        }

        case 3: {
          _localctx = _tracker.createInstance<ConstrContext>(parentContext, parentState);
          pushNewRecursionContext(_localctx, startState, RuleConstr);
          setState(320);

          if (!(precpred(_ctx, 6))) throw FailedPredicateException(this, "precpred(_ctx, 6)");
          setState(323);
          _errHandler->sync(this);
          switch (_input->LA(1)) {
            case CrySLParser::T__39: {
              setState(321);
              dynamic_cast<ConstrContext *>(_localctx)->plus = match(CrySLParser::T__39);
              break;
            }

            case CrySLParser::T__40: {
              setState(322);
              dynamic_cast<ConstrContext *>(_localctx)->minus = match(CrySLParser::T__40);
              break;
            }

          default:
            throw NoViableAltException(this);
          }
          setState(325);
          constr(7);
          break;
        }

        case 4: {
          _localctx = _tracker.createInstance<ConstrContext>(parentContext, parentState);
          pushNewRecursionContext(_localctx, startState, RuleConstr);
          setState(326);

          if (!(precpred(_ctx, 5))) throw FailedPredicateException(this, "precpred(_ctx, 5)");
          setState(327);
          comparingRelOperator();
          setState(328);
          constr(6);
          break;
        }

        case 5: {
          _localctx = _tracker.createInstance<ConstrContext>(parentContext, parentState);
          pushNewRecursionContext(_localctx, startState, RuleConstr);
          setState(330);

          if (!(precpred(_ctx, 4))) throw FailedPredicateException(this, "precpred(_ctx, 4)");
          setState(333);
          _errHandler->sync(this);
          switch (_input->LA(1)) {
            case CrySLParser::T__41: {
              setState(331);
              dynamic_cast<ConstrContext *>(_localctx)->equal = match(CrySLParser::T__41);
              break;
            }

            case CrySLParser::T__42: {
              setState(332);
              dynamic_cast<ConstrContext *>(_localctx)->unequal = match(CrySLParser::T__42);
              break;
            }

          default:
            throw NoViableAltException(this);
          }
          setState(335);
          constr(5);
          break;
        }

        case 6: {
          _localctx = _tracker.createInstance<ConstrContext>(parentContext, parentState);
          pushNewRecursionContext(_localctx, startState, RuleConstr);
          setState(336);

          if (!(precpred(_ctx, 3))) throw FailedPredicateException(this, "precpred(_ctx, 3)");
          setState(337);
          dynamic_cast<ConstrContext *>(_localctx)->land = match(CrySLParser::T__43);
          setState(338);
          constr(4);
          break;
        }

        case 7: {
          _localctx = _tracker.createInstance<ConstrContext>(parentContext, parentState);
          pushNewRecursionContext(_localctx, startState, RuleConstr);
          setState(339);

          if (!(precpred(_ctx, 2))) throw FailedPredicateException(this, "precpred(_ctx, 2)");
          setState(340);
          dynamic_cast<ConstrContext *>(_localctx)->lor = match(CrySLParser::T__20);
          setState(341);
          constr(3);
          break;
        }

        case 8: {
          _localctx = _tracker.createInstance<ConstrContext>(parentContext, parentState);
          pushNewRecursionContext(_localctx, startState, RuleConstr);
          setState(342);

          if (!(precpred(_ctx, 1))) throw FailedPredicateException(this, "precpred(_ctx, 1)");
          setState(343);
          dynamic_cast<ConstrContext *>(_localctx)->implies = match(CrySLParser::T__21);
          setState(344);
          constr(1);
          break;
        }

        } 
      }
      setState(349);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 35, _ctx);
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
  enterRule(_localctx, 48, CrySLParser::RuleComparingRelOperator);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    setState(354);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case CrySLParser::T__44: {
        enterOuterAlt(_localctx, 1);
        setState(350);
        dynamic_cast<ComparingRelOperatorContext *>(_localctx)->less = match(CrySLParser::T__44);
        break;
      }

      case CrySLParser::T__45: {
        enterOuterAlt(_localctx, 2);
        setState(351);
        dynamic_cast<ComparingRelOperatorContext *>(_localctx)->less_or_equal = match(CrySLParser::T__45);
        break;
      }

      case CrySLParser::T__46: {
        enterOuterAlt(_localctx, 3);
        setState(352);
        dynamic_cast<ComparingRelOperatorContext *>(_localctx)->greater_or_equal = match(CrySLParser::T__46);
        break;
      }

      case CrySLParser::T__47: {
        enterOuterAlt(_localctx, 4);
        setState(353);
        dynamic_cast<ComparingRelOperatorContext *>(_localctx)->greater = match(CrySLParser::T__47);
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
  enterRule(_localctx, 50, CrySLParser::RuleCons);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    setState(363);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 37, _ctx)) {
    case 1: {
      enterOuterAlt(_localctx, 1);
      setState(356);
      arrayElements();
      setState(357);
      match(CrySLParser::T__48);
      setState(358);
      match(CrySLParser::T__49);
      setState(359);
      litList();
      setState(360);
      match(CrySLParser::T__50);
      break;
    }

    case 2: {
      enterOuterAlt(_localctx, 2);
      setState(362);
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
  enterRule(_localctx, 52, CrySLParser::RuleArrayElements);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    setState(371);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case CrySLParser::T__51: {
        enterOuterAlt(_localctx, 1);
        setState(365);
        dynamic_cast<ArrayElementsContext *>(_localctx)->el = match(CrySLParser::T__51);
        setState(366);
        match(CrySLParser::T__35);
        setState(367);
        consPred();
        setState(368);
        match(CrySLParser::T__36);
        break;
      }

      case CrySLParser::T__14:
      case CrySLParser::T__27:
      case CrySLParser::T__28:
      case CrySLParser::T__29:
      case CrySLParser::T__30:
      case CrySLParser::T__31:
      case CrySLParser::Int:
      case CrySLParser::Bool:
      case CrySLParser::String:
      case CrySLParser::Ident: {
        enterOuterAlt(_localctx, 2);
        setState(370);
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
  enterRule(_localctx, 54, CrySLParser::RuleLitList);
  size_t _la = 0;

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(373);
    literal();
    setState(381);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == CrySLParser::T__19) {
      setState(374);
      match(CrySLParser::T__19);
      setState(377);
      _errHandler->sync(this);
      switch (_input->LA(1)) {
        case CrySLParser::Int:
        case CrySLParser::Bool:
        case CrySLParser::String: {
          setState(375);
          literal();
          break;
        }

        case CrySLParser::T__52: {
          setState(376);
          dynamic_cast<LitListContext *>(_localctx)->ellipsis = match(CrySLParser::T__52);
          break;
        }

      default:
        throw NoViableAltException(this);
      }
      setState(383);
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
  enterRule(_localctx, 56, CrySLParser::RuleEvents);
  size_t _la = 0;

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(384);
    match(CrySLParser::T__53);
    setState(386); 
    _errHandler->sync(this);
    _la = _input->LA(1);
    do {
      setState(385);
      eventsOccurence();
      setState(388); 
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
  enterRule(_localctx, 58, CrySLParser::RuleEventsOccurence);
  size_t _la = 0;

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(390);
    dynamic_cast<EventsOccurenceContext *>(_localctx)->eventName = match(CrySLParser::Ident);
    setState(391);
    match(CrySLParser::T__54);
    setState(394);
    _errHandler->sync(this);

    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 42, _ctx)) {
    case 1: {
      setState(392);
      dynamic_cast<EventsOccurenceContext *>(_localctx)->returnValue = match(CrySLParser::Ident);
      setState(393);
      match(CrySLParser::T__55);
      break;
    }

    }
    setState(396);
    dynamic_cast<EventsOccurenceContext *>(_localctx)->methodName = match(CrySLParser::Ident);
    setState(397);
    match(CrySLParser::T__35);
    setState(399);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (((((_la - 15) & ~ 0x3fULL) == 0) &&
      ((1ULL << (_la - 15)) & ((1ULL << (CrySLParser::T__14 - 15))
      | (1ULL << (CrySLParser::T__22 - 15))
      | (1ULL << (CrySLParser::T__23 - 15))
      | (1ULL << (CrySLParser::Ident - 15)))) != 0)) {
      setState(398);
      parametersList();
    }
    setState(401);
    match(CrySLParser::T__36);
    setState(402);
    match(CrySLParser::T__4);
   
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
  enterRule(_localctx, 60, CrySLParser::RuleParametersList);
  size_t _la = 0;

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(404);
    param();
    setState(409);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == CrySLParser::T__19) {
      setState(405);
      match(CrySLParser::T__19);
      setState(406);
      param();
      setState(411);
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
  enterRule(_localctx, 62, CrySLParser::RuleParam);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    setState(415);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case CrySLParser::T__14:
      case CrySLParser::Ident: {
        enterOuterAlt(_localctx, 1);
        setState(412);
        memberAccess();
        break;
      }

      case CrySLParser::T__22: {
        enterOuterAlt(_localctx, 2);
        setState(413);
        dynamic_cast<ParamContext *>(_localctx)->thisPtr = match(CrySLParser::T__22);
        break;
      }

      case CrySLParser::T__23: {
        enterOuterAlt(_localctx, 3);
        setState(414);
        dynamic_cast<ParamContext *>(_localctx)->wildCard = match(CrySLParser::T__23);
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
  enterRule(_localctx, 64, CrySLParser::RuleOrder);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(417);
    match(CrySLParser::T__56);
    setState(418);
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
  enterRule(_localctx, 66, CrySLParser::RuleOrderSequence);
  size_t _la = 0;

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(420);
    simpleOrder();
    setState(425);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == CrySLParser::T__19) {
      setState(421);
      match(CrySLParser::T__19);
      setState(422);
      simpleOrder();
      setState(427);
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

std::vector<CrySLParser::UnorderedSymbolsContext *> CrySLParser::SimpleOrderContext::unorderedSymbols() {
  return getRuleContexts<CrySLParser::UnorderedSymbolsContext>();
}

CrySLParser::UnorderedSymbolsContext* CrySLParser::SimpleOrderContext::unorderedSymbols(size_t i) {
  return getRuleContext<CrySLParser::UnorderedSymbolsContext>(i);
}


size_t CrySLParser::SimpleOrderContext::getRuleIndex() const {
  return CrySLParser::RuleSimpleOrder;
}


CrySLParser::SimpleOrderContext* CrySLParser::simpleOrder() {
  SimpleOrderContext *_localctx = _tracker.createInstance<SimpleOrderContext>(_ctx, getState());
  enterRule(_localctx, 68, CrySLParser::RuleSimpleOrder);
  size_t _la = 0;

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(428);
    unorderedSymbols();
    setState(433);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == CrySLParser::T__57) {
      setState(429);
      match(CrySLParser::T__57);
      setState(430);
      unorderedSymbols();
      setState(435);
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

//----------------- UnorderedSymbolsContext ------------------------------------------------------------------

CrySLParser::UnorderedSymbolsContext::UnorderedSymbolsContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<CrySLParser::PrimaryContext *> CrySLParser::UnorderedSymbolsContext::primary() {
  return getRuleContexts<CrySLParser::PrimaryContext>();
}

CrySLParser::PrimaryContext* CrySLParser::UnorderedSymbolsContext::primary(size_t i) {
  return getRuleContext<CrySLParser::PrimaryContext>(i);
}

std::vector<tree::TerminalNode *> CrySLParser::UnorderedSymbolsContext::Int() {
  return getTokens(CrySLParser::Int);
}

tree::TerminalNode* CrySLParser::UnorderedSymbolsContext::Int(size_t i) {
  return getToken(CrySLParser::Int, i);
}


size_t CrySLParser::UnorderedSymbolsContext::getRuleIndex() const {
  return CrySLParser::RuleUnorderedSymbols;
}


CrySLParser::UnorderedSymbolsContext* CrySLParser::unorderedSymbols() {
  UnorderedSymbolsContext *_localctx = _tracker.createInstance<UnorderedSymbolsContext>(_ctx, getState());
  enterRule(_localctx, 70, CrySLParser::RuleUnorderedSymbols);
  size_t _la = 0;

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(436);
    primary();
    setState(452);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == CrySLParser::T__58) {
      setState(439); 
      _errHandler->sync(this);
      _la = _input->LA(1);
      do {
        setState(437);
        match(CrySLParser::T__58);
        setState(438);
        primary();
        setState(441); 
        _errHandler->sync(this);
        _la = _input->LA(1);
      } while (_la == CrySLParser::T__58);
      setState(450);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == CrySLParser::T__59

      || _la == CrySLParser::Int) {
        setState(444);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (_la == CrySLParser::Int) {
          setState(443);
          dynamic_cast<UnorderedSymbolsContext *>(_localctx)->lower = match(CrySLParser::Int);
        }
        setState(446);
        match(CrySLParser::T__59);
        setState(448);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (_la == CrySLParser::Int) {
          setState(447);
          dynamic_cast<UnorderedSymbolsContext *>(_localctx)->upper = match(CrySLParser::Int);
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
  enterRule(_localctx, 72, CrySLParser::RulePrimary);
  size_t _la = 0;

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    setState(464);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case CrySLParser::Ident: {
        enterOuterAlt(_localctx, 1);
        setState(454);
        dynamic_cast<PrimaryContext *>(_localctx)->eventName = match(CrySLParser::Ident);
        setState(456);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if ((((_la & ~ 0x3fULL) == 0) &&
          ((1ULL << _la) & ((1ULL << CrySLParser::T__14)
          | (1ULL << CrySLParser::T__39)
          | (1ULL << CrySLParser::T__60))) != 0)) {
          setState(455);
          dynamic_cast<PrimaryContext *>(_localctx)->elementop = _input->LT(1);
          _la = _input->LA(1);
          if (!((((_la & ~ 0x3fULL) == 0) &&
            ((1ULL << _la) & ((1ULL << CrySLParser::T__14)
            | (1ULL << CrySLParser::T__39)
            | (1ULL << CrySLParser::T__60))) != 0))) {
            dynamic_cast<PrimaryContext *>(_localctx)->elementop = _errHandler->recoverInline(this);
          }
          else {
            _errHandler->reportMatch(this);
            consume();
          }
        }
        break;
      }

      case CrySLParser::T__35: {
        enterOuterAlt(_localctx, 2);
        setState(458);
        match(CrySLParser::T__35);
        setState(459);
        orderSequence();
        setState(460);
        match(CrySLParser::T__36);
        setState(462);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if ((((_la & ~ 0x3fULL) == 0) &&
          ((1ULL << _la) & ((1ULL << CrySLParser::T__14)
          | (1ULL << CrySLParser::T__39)
          | (1ULL << CrySLParser::T__60))) != 0)) {
          setState(461);
          dynamic_cast<PrimaryContext *>(_localctx)->elementop = _input->LT(1);
          _la = _input->LA(1);
          if (!((((_la & ~ 0x3fULL) == 0) &&
            ((1ULL << _la) & ((1ULL << CrySLParser::T__14)
            | (1ULL << CrySLParser::T__39)
            | (1ULL << CrySLParser::T__60))) != 0))) {
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
  enterRule(_localctx, 74, CrySLParser::RuleNegates);
  size_t _la = 0;

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(466);
    match(CrySLParser::T__61);
    setState(468); 
    _errHandler->sync(this);
    _la = _input->LA(1);
    do {
      setState(467);
      negatesOccurence();
      setState(470); 
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
  enterRule(_localctx, 76, CrySLParser::RuleNegatesOccurence);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(472);
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
  enterRule(_localctx, 78, CrySLParser::RuleForbidden);
  size_t _la = 0;

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(474);
    match(CrySLParser::T__62);
    setState(476); 
    _errHandler->sync(this);
    _la = _input->LA(1);
    do {
      setState(475);
      forbiddenOccurence();
      setState(478); 
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
  enterRule(_localctx, 80, CrySLParser::RuleForbiddenOccurence);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(480);
    dynamic_cast<ForbiddenOccurenceContext *>(_localctx)->methodName = fqn();

    setState(481);
    match(CrySLParser::T__21);
    setState(482);
    dynamic_cast<ForbiddenOccurenceContext *>(_localctx)->eventName = match(CrySLParser::Ident);
    setState(484);
    match(CrySLParser::T__4);
   
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
  enterRule(_localctx, 82, CrySLParser::RuleFqn);
  size_t _la = 0;

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(486);
    qualifiedName();
    setState(487);
    match(CrySLParser::T__35);
    setState(489);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (((((_la - 6) & ~ 0x3fULL) == 0) &&
      ((1ULL << (_la - 6)) & ((1ULL << (CrySLParser::T__5 - 6))
      | (1ULL << (CrySLParser::T__6 - 6))
      | (1ULL << (CrySLParser::T__7 - 6))
      | (1ULL << (CrySLParser::T__8 - 6))
      | (1ULL << (CrySLParser::T__9 - 6))
      | (1ULL << (CrySLParser::T__10 - 6))
      | (1ULL << (CrySLParser::T__11 - 6))
      | (1ULL << (CrySLParser::T__12 - 6))
      | (1ULL << (CrySLParser::T__13 - 6))
      | (1ULL << (CrySLParser::Ident - 6)))) != 0)) {
      setState(488);
      typeNameList();
    }
    setState(491);
    match(CrySLParser::T__36);
   
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
  enterRule(_localctx, 84, CrySLParser::RuleTypeNameList);
  size_t _la = 0;

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(493);
    typeName();
    setState(498);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == CrySLParser::T__19) {
      setState(494);
      match(CrySLParser::T__19);
      setState(495);
      typeName();
      setState(500);
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
    case 10: return reqPredSempred(dynamic_cast<ReqPredContext *>(context), predicateIndex);
    case 23: return constrSempred(dynamic_cast<ConstrContext *>(context), predicateIndex);

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
  "domainModel", "spec", "qualifiedName", "objects", "objectDecl", "primitiveTypeName", 
  "typeName", "ptr", "array", "requiresBlock", "reqPred", "reqPredLit", 
  "pred", "suParList", "suPar", "consPred", "literalExpr", "literal", "memberAccess", 
  "preDefinedPredicate", "ensures", "ensPred", "constraints", "constr", 
  "comparingRelOperator", "cons", "arrayElements", "litList", "events", 
  "eventsOccurence", "parametersList", "param", "order", "orderSequence", 
  "simpleOrder", "unorderedSymbols", "primary", "negates", "negatesOccurence", 
  "forbidden", "forbiddenOccurence", "fqn", "typeNameList"
};

std::vector<std::string> CrySLParser::_literalNames = {
  "", "'SPEC'", "'::'", "'OBJECTS'", "'const'", "';'", "'bool'", "'unsigned'", 
  "'char'", "'short'", "'int'", "'long'", "'float'", "'double'", "'size_t'", 
  "'*'", "'['", "']'", "'REQUIRES'", "'!'", "','", "'||'", "'=>'", "'this'", 
  "'_'", "'^'", "'.'", "'->'", "'neverTypeOf'", "'noCallTo'", "'callTo'", 
  "'notHardCoded'", "'length'", "'ENSURES'", "'after'", "'CONSTRAINTS'", 
  "'('", "')'", "'/'", "'%'", "'+'", "'-'", "'=='", "'!='", "'&&'", "'<'", 
  "'<='", "'>='", "'>'", "'in'", "'{'", "'}'", "'elements'", "'...'", "'EVENTS'", 
  "':'", "'='", "'ORDER'", "'|'", "'~'", "'#'", "'?'", "'NEGATES'", "'FORBIDDEN'"
};

std::vector<std::string> CrySLParser::_symbolicNames = {
  "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", 
  "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", 
  "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", 
  "", "", "", "", "", "", "", "", "", "", "Int", "Double", "Char", "Bool", 
  "String", "Ident", "COMMENT", "LINE_COMMENT", "WS"
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
    0x3, 0x4a, 0x1f8, 0x4, 0x2, 0x9, 0x2, 0x4, 0x3, 0x9, 0x3, 0x4, 0x4, 
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
    0x4, 0x29, 0x9, 0x29, 0x4, 0x2a, 0x9, 0x2a, 0x4, 0x2b, 0x9, 0x2b, 0x4, 
    0x2c, 0x9, 0x2c, 0x3, 0x2, 0x3, 0x2, 0x3, 0x2, 0x5, 0x2, 0x5c, 0xa, 
    0x2, 0x3, 0x2, 0x3, 0x2, 0x3, 0x2, 0x5, 0x2, 0x61, 0xa, 0x2, 0x3, 0x2, 
    0x5, 0x2, 0x64, 0xa, 0x2, 0x3, 0x2, 0x5, 0x2, 0x67, 0xa, 0x2, 0x3, 0x2, 
    0x5, 0x2, 0x6a, 0xa, 0x2, 0x3, 0x2, 0x3, 0x2, 0x3, 0x3, 0x3, 0x3, 0x3, 
    0x3, 0x3, 0x4, 0x3, 0x4, 0x3, 0x4, 0x7, 0x4, 0x74, 0xa, 0x4, 0xc, 0x4, 
    0xe, 0x4, 0x77, 0xb, 0x4, 0x3, 0x5, 0x3, 0x5, 0x7, 0x5, 0x7b, 0xa, 0x5, 
    0xc, 0x5, 0xe, 0x5, 0x7e, 0xb, 0x5, 0x3, 0x6, 0x5, 0x6, 0x81, 0xa, 0x6, 
    0x3, 0x6, 0x3, 0x6, 0x3, 0x6, 0x7, 0x6, 0x86, 0xa, 0x6, 0xc, 0x6, 0xe, 
    0x6, 0x89, 0xb, 0x6, 0x3, 0x6, 0x3, 0x6, 0x3, 0x7, 0x3, 0x7, 0x5, 0x7, 
    0x8f, 0xa, 0x7, 0x3, 0x7, 0x3, 0x7, 0x3, 0x7, 0x3, 0x7, 0x3, 0x7, 0x3, 
    0x7, 0x5, 0x7, 0x97, 0xa, 0x7, 0x3, 0x7, 0x3, 0x7, 0x5, 0x7, 0x9b, 0xa, 
    0x7, 0x3, 0x7, 0x3, 0x7, 0x5, 0x7, 0x9f, 0xa, 0x7, 0x3, 0x8, 0x3, 0x8, 
    0x5, 0x8, 0xa3, 0xa, 0x8, 0x3, 0x8, 0x7, 0x8, 0xa6, 0xa, 0x8, 0xc, 0x8, 
    0xe, 0x8, 0xa9, 0xb, 0x8, 0x3, 0x9, 0x3, 0x9, 0x3, 0xa, 0x3, 0xa, 0x3, 
    0xa, 0x3, 0xa, 0x3, 0xb, 0x3, 0xb, 0x3, 0xb, 0x3, 0xb, 0x7, 0xb, 0xb5, 
    0xa, 0xb, 0xc, 0xb, 0xe, 0xb, 0xb8, 0xb, 0xb, 0x3, 0xc, 0x3, 0xc, 0x5, 
    0xc, 0xbc, 0xa, 0xc, 0x3, 0xc, 0x3, 0xc, 0x3, 0xc, 0x3, 0xc, 0x3, 0xc, 
    0x5, 0xc, 0xc3, 0xa, 0xc, 0x3, 0xc, 0x3, 0xc, 0x3, 0xc, 0x3, 0xc, 0x3, 
    0xc, 0x3, 0xc, 0x7, 0xc, 0xcb, 0xa, 0xc, 0xc, 0xc, 0xe, 0xc, 0xce, 0xb, 
    0xc, 0x3, 0xd, 0x3, 0xd, 0x3, 0xe, 0x3, 0xe, 0x3, 0xe, 0x5, 0xe, 0xd5, 
    0xa, 0xe, 0x3, 0xe, 0x3, 0xe, 0x3, 0xf, 0x3, 0xf, 0x3, 0xf, 0x7, 0xf, 
    0xdc, 0xa, 0xf, 0xc, 0xf, 0xe, 0xf, 0xdf, 0xb, 0xf, 0x3, 0x10, 0x3, 
    0x10, 0x3, 0x10, 0x5, 0x10, 0xe4, 0xa, 0x10, 0x3, 0x11, 0x3, 0x11, 0x3, 
    0x12, 0x3, 0x12, 0x3, 0x12, 0x5, 0x12, 0xeb, 0xa, 0x12, 0x3, 0x13, 0x3, 
    0x13, 0x3, 0x13, 0x3, 0x13, 0x3, 0x13, 0x3, 0x13, 0x5, 0x13, 0xf3, 0xa, 
    0x13, 0x3, 0x14, 0x3, 0x14, 0x3, 0x14, 0x3, 0x14, 0x3, 0x14, 0x3, 0x14, 
    0x3, 0x14, 0x3, 0x14, 0x3, 0x14, 0x5, 0x14, 0xfe, 0xa, 0x14, 0x3, 0x15, 
    0x3, 0x15, 0x3, 0x15, 0x3, 0x15, 0x3, 0x15, 0x3, 0x15, 0x3, 0x15, 0x3, 
    0x15, 0x3, 0x15, 0x3, 0x15, 0x3, 0x15, 0x3, 0x15, 0x3, 0x15, 0x3, 0x15, 
    0x3, 0x15, 0x3, 0x15, 0x3, 0x15, 0x3, 0x15, 0x3, 0x15, 0x3, 0x15, 0x3, 
    0x15, 0x3, 0x15, 0x3, 0x15, 0x3, 0x15, 0x3, 0x15, 0x5, 0x15, 0x119, 
    0xa, 0x15, 0x3, 0x16, 0x3, 0x16, 0x3, 0x16, 0x3, 0x16, 0x6, 0x16, 0x11f, 
    0xa, 0x16, 0xd, 0x16, 0xe, 0x16, 0x120, 0x3, 0x17, 0x3, 0x17, 0x3, 0x17, 
    0x5, 0x17, 0x126, 0xa, 0x17, 0x3, 0x18, 0x3, 0x18, 0x3, 0x18, 0x3, 0x18, 
    0x6, 0x18, 0x12c, 0xa, 0x18, 0xd, 0x18, 0xe, 0x18, 0x12d, 0x3, 0x19, 
    0x3, 0x19, 0x3, 0x19, 0x3, 0x19, 0x3, 0x19, 0x3, 0x19, 0x3, 0x19, 0x3, 
    0x19, 0x5, 0x19, 0x138, 0xa, 0x19, 0x3, 0x19, 0x3, 0x19, 0x3, 0x19, 
    0x5, 0x19, 0x13d, 0xa, 0x19, 0x3, 0x19, 0x3, 0x19, 0x3, 0x19, 0x3, 0x19, 
    0x3, 0x19, 0x3, 0x19, 0x3, 0x19, 0x5, 0x19, 0x146, 0xa, 0x19, 0x3, 0x19, 
    0x3, 0x19, 0x3, 0x19, 0x3, 0x19, 0x3, 0x19, 0x3, 0x19, 0x3, 0x19, 0x3, 
    0x19, 0x5, 0x19, 0x150, 0xa, 0x19, 0x3, 0x19, 0x3, 0x19, 0x3, 0x19, 
    0x3, 0x19, 0x3, 0x19, 0x3, 0x19, 0x3, 0x19, 0x3, 0x19, 0x3, 0x19, 0x3, 
    0x19, 0x7, 0x19, 0x15c, 0xa, 0x19, 0xc, 0x19, 0xe, 0x19, 0x15f, 0xb, 
    0x19, 0x3, 0x1a, 0x3, 0x1a, 0x3, 0x1a, 0x3, 0x1a, 0x5, 0x1a, 0x165, 
    0xa, 0x1a, 0x3, 0x1b, 0x3, 0x1b, 0x3, 0x1b, 0x3, 0x1b, 0x3, 0x1b, 0x3, 
    0x1b, 0x3, 0x1b, 0x5, 0x1b, 0x16e, 0xa, 0x1b, 0x3, 0x1c, 0x3, 0x1c, 
    0x3, 0x1c, 0x3, 0x1c, 0x3, 0x1c, 0x3, 0x1c, 0x5, 0x1c, 0x176, 0xa, 0x1c, 
    0x3, 0x1d, 0x3, 0x1d, 0x3, 0x1d, 0x3, 0x1d, 0x5, 0x1d, 0x17c, 0xa, 0x1d, 
    0x7, 0x1d, 0x17e, 0xa, 0x1d, 0xc, 0x1d, 0xe, 0x1d, 0x181, 0xb, 0x1d, 
    0x3, 0x1e, 0x3, 0x1e, 0x6, 0x1e, 0x185, 0xa, 0x1e, 0xd, 0x1e, 0xe, 0x1e, 
    0x186, 0x3, 0x1f, 0x3, 0x1f, 0x3, 0x1f, 0x3, 0x1f, 0x5, 0x1f, 0x18d, 
    0xa, 0x1f, 0x3, 0x1f, 0x3, 0x1f, 0x3, 0x1f, 0x5, 0x1f, 0x192, 0xa, 0x1f, 
    0x3, 0x1f, 0x3, 0x1f, 0x3, 0x1f, 0x3, 0x20, 0x3, 0x20, 0x3, 0x20, 0x7, 
    0x20, 0x19a, 0xa, 0x20, 0xc, 0x20, 0xe, 0x20, 0x19d, 0xb, 0x20, 0x3, 
    0x21, 0x3, 0x21, 0x3, 0x21, 0x5, 0x21, 0x1a2, 0xa, 0x21, 0x3, 0x22, 
    0x3, 0x22, 0x3, 0x22, 0x3, 0x23, 0x3, 0x23, 0x3, 0x23, 0x7, 0x23, 0x1aa, 
    0xa, 0x23, 0xc, 0x23, 0xe, 0x23, 0x1ad, 0xb, 0x23, 0x3, 0x24, 0x3, 0x24, 
    0x3, 0x24, 0x7, 0x24, 0x1b2, 0xa, 0x24, 0xc, 0x24, 0xe, 0x24, 0x1b5, 
    0xb, 0x24, 0x3, 0x25, 0x3, 0x25, 0x3, 0x25, 0x6, 0x25, 0x1ba, 0xa, 0x25, 
    0xd, 0x25, 0xe, 0x25, 0x1bb, 0x3, 0x25, 0x5, 0x25, 0x1bf, 0xa, 0x25, 
    0x3, 0x25, 0x3, 0x25, 0x5, 0x25, 0x1c3, 0xa, 0x25, 0x5, 0x25, 0x1c5, 
    0xa, 0x25, 0x5, 0x25, 0x1c7, 0xa, 0x25, 0x3, 0x26, 0x3, 0x26, 0x5, 0x26, 
    0x1cb, 0xa, 0x26, 0x3, 0x26, 0x3, 0x26, 0x3, 0x26, 0x3, 0x26, 0x5, 0x26, 
    0x1d1, 0xa, 0x26, 0x5, 0x26, 0x1d3, 0xa, 0x26, 0x3, 0x27, 0x3, 0x27, 
    0x6, 0x27, 0x1d7, 0xa, 0x27, 0xd, 0x27, 0xe, 0x27, 0x1d8, 0x3, 0x28, 
    0x3, 0x28, 0x3, 0x29, 0x3, 0x29, 0x6, 0x29, 0x1df, 0xa, 0x29, 0xd, 0x29, 
    0xe, 0x29, 0x1e0, 0x3, 0x2a, 0x3, 0x2a, 0x3, 0x2a, 0x3, 0x2a, 0x3, 0x2a, 
    0x3, 0x2a, 0x3, 0x2b, 0x3, 0x2b, 0x3, 0x2b, 0x5, 0x2b, 0x1ec, 0xa, 0x2b, 
    0x3, 0x2b, 0x3, 0x2b, 0x3, 0x2c, 0x3, 0x2c, 0x3, 0x2c, 0x7, 0x2c, 0x1f3, 
    0xa, 0x2c, 0xc, 0x2c, 0xe, 0x2c, 0x1f6, 0xb, 0x2c, 0x3, 0x2c, 0x2, 0x4, 
    0x16, 0x30, 0x2d, 0x2, 0x4, 0x6, 0x8, 0xa, 0xc, 0xe, 0x10, 0x12, 0x14, 
    0x16, 0x18, 0x1a, 0x1c, 0x1e, 0x20, 0x22, 0x24, 0x26, 0x28, 0x2a, 0x2c, 
    0x2e, 0x30, 0x32, 0x34, 0x36, 0x38, 0x3a, 0x3c, 0x3e, 0x40, 0x42, 0x44, 
    0x46, 0x48, 0x4a, 0x4c, 0x4e, 0x50, 0x52, 0x54, 0x56, 0x2, 0x3, 0x5, 
    0x2, 0x11, 0x11, 0x2a, 0x2a, 0x3f, 0x3f, 0x2, 0x222, 0x2, 0x58, 0x3, 
    0x2, 0x2, 0x2, 0x4, 0x6d, 0x3, 0x2, 0x2, 0x2, 0x6, 0x70, 0x3, 0x2, 0x2, 
    0x2, 0x8, 0x78, 0x3, 0x2, 0x2, 0x2, 0xa, 0x80, 0x3, 0x2, 0x2, 0x2, 0xc, 
    0x9e, 0x3, 0x2, 0x2, 0x2, 0xe, 0xa2, 0x3, 0x2, 0x2, 0x2, 0x10, 0xaa, 
    0x3, 0x2, 0x2, 0x2, 0x12, 0xac, 0x3, 0x2, 0x2, 0x2, 0x14, 0xb0, 0x3, 
    0x2, 0x2, 0x2, 0x16, 0xc2, 0x3, 0x2, 0x2, 0x2, 0x18, 0xcf, 0x3, 0x2, 
    0x2, 0x2, 0x1a, 0xd1, 0x3, 0x2, 0x2, 0x2, 0x1c, 0xd8, 0x3, 0x2, 0x2, 
    0x2, 0x1e, 0xe3, 0x3, 0x2, 0x2, 0x2, 0x20, 0xe5, 0x3, 0x2, 0x2, 0x2, 
    0x22, 0xea, 0x3, 0x2, 0x2, 0x2, 0x24, 0xf2, 0x3, 0x2, 0x2, 0x2, 0x26, 
    0xfd, 0x3, 0x2, 0x2, 0x2, 0x28, 0x118, 0x3, 0x2, 0x2, 0x2, 0x2a, 0x11a, 
    0x3, 0x2, 0x2, 0x2, 0x2c, 0x122, 0x3, 0x2, 0x2, 0x2, 0x2e, 0x127, 0x3, 
    0x2, 0x2, 0x2, 0x30, 0x137, 0x3, 0x2, 0x2, 0x2, 0x32, 0x164, 0x3, 0x2, 
    0x2, 0x2, 0x34, 0x16d, 0x3, 0x2, 0x2, 0x2, 0x36, 0x175, 0x3, 0x2, 0x2, 
    0x2, 0x38, 0x177, 0x3, 0x2, 0x2, 0x2, 0x3a, 0x182, 0x3, 0x2, 0x2, 0x2, 
    0x3c, 0x188, 0x3, 0x2, 0x2, 0x2, 0x3e, 0x196, 0x3, 0x2, 0x2, 0x2, 0x40, 
    0x1a1, 0x3, 0x2, 0x2, 0x2, 0x42, 0x1a3, 0x3, 0x2, 0x2, 0x2, 0x44, 0x1a6, 
    0x3, 0x2, 0x2, 0x2, 0x46, 0x1ae, 0x3, 0x2, 0x2, 0x2, 0x48, 0x1b6, 0x3, 
    0x2, 0x2, 0x2, 0x4a, 0x1d2, 0x3, 0x2, 0x2, 0x2, 0x4c, 0x1d4, 0x3, 0x2, 
    0x2, 0x2, 0x4e, 0x1da, 0x3, 0x2, 0x2, 0x2, 0x50, 0x1dc, 0x3, 0x2, 0x2, 
    0x2, 0x52, 0x1e2, 0x3, 0x2, 0x2, 0x2, 0x54, 0x1e8, 0x3, 0x2, 0x2, 0x2, 
    0x56, 0x1ef, 0x3, 0x2, 0x2, 0x2, 0x58, 0x59, 0x5, 0x4, 0x3, 0x2, 0x59, 
    0x5b, 0x5, 0x8, 0x5, 0x2, 0x5a, 0x5c, 0x5, 0x50, 0x29, 0x2, 0x5b, 0x5a, 
    0x3, 0x2, 0x2, 0x2, 0x5b, 0x5c, 0x3, 0x2, 0x2, 0x2, 0x5c, 0x5d, 0x3, 
    0x2, 0x2, 0x2, 0x5d, 0x5e, 0x5, 0x3a, 0x1e, 0x2, 0x5e, 0x60, 0x5, 0x42, 
    0x22, 0x2, 0x5f, 0x61, 0x5, 0x2e, 0x18, 0x2, 0x60, 0x5f, 0x3, 0x2, 0x2, 
    0x2, 0x60, 0x61, 0x3, 0x2, 0x2, 0x2, 0x61, 0x63, 0x3, 0x2, 0x2, 0x2, 
    0x62, 0x64, 0x5, 0x14, 0xb, 0x2, 0x63, 0x62, 0x3, 0x2, 0x2, 0x2, 0x63, 
    0x64, 0x3, 0x2, 0x2, 0x2, 0x64, 0x66, 0x3, 0x2, 0x2, 0x2, 0x65, 0x67, 
    0x5, 0x2a, 0x16, 0x2, 0x66, 0x65, 0x3, 0x2, 0x2, 0x2, 0x66, 0x67, 0x3, 
    0x2, 0x2, 0x2, 0x67, 0x69, 0x3, 0x2, 0x2, 0x2, 0x68, 0x6a, 0x5, 0x4c, 
    0x27, 0x2, 0x69, 0x68, 0x3, 0x2, 0x2, 0x2, 0x69, 0x6a, 0x3, 0x2, 0x2, 
    0x2, 0x6a, 0x6b, 0x3, 0x2, 0x2, 0x2, 0x6b, 0x6c, 0x7, 0x2, 0x2, 0x3, 
    0x6c, 0x3, 0x3, 0x2, 0x2, 0x2, 0x6d, 0x6e, 0x7, 0x3, 0x2, 0x2, 0x6e, 
    0x6f, 0x5, 0x6, 0x4, 0x2, 0x6f, 0x5, 0x3, 0x2, 0x2, 0x2, 0x70, 0x75, 
    0x7, 0x47, 0x2, 0x2, 0x71, 0x72, 0x7, 0x4, 0x2, 0x2, 0x72, 0x74, 0x7, 
    0x47, 0x2, 0x2, 0x73, 0x71, 0x3, 0x2, 0x2, 0x2, 0x74, 0x77, 0x3, 0x2, 
    0x2, 0x2, 0x75, 0x73, 0x3, 0x2, 0x2, 0x2, 0x75, 0x76, 0x3, 0x2, 0x2, 
    0x2, 0x76, 0x7, 0x3, 0x2, 0x2, 0x2, 0x77, 0x75, 0x3, 0x2, 0x2, 0x2, 
    0x78, 0x7c, 0x7, 0x5, 0x2, 0x2, 0x79, 0x7b, 0x5, 0xa, 0x6, 0x2, 0x7a, 
    0x79, 0x3, 0x2, 0x2, 0x2, 0x7b, 0x7e, 0x3, 0x2, 0x2, 0x2, 0x7c, 0x7a, 
    0x3, 0x2, 0x2, 0x2, 0x7c, 0x7d, 0x3, 0x2, 0x2, 0x2, 0x7d, 0x9, 0x3, 
    0x2, 0x2, 0x2, 0x7e, 0x7c, 0x3, 0x2, 0x2, 0x2, 0x7f, 0x81, 0x7, 0x6, 
    0x2, 0x2, 0x80, 0x7f, 0x3, 0x2, 0x2, 0x2, 0x80, 0x81, 0x3, 0x2, 0x2, 
    0x2, 0x81, 0x82, 0x3, 0x2, 0x2, 0x2, 0x82, 0x83, 0x5, 0xe, 0x8, 0x2, 
    0x83, 0x87, 0x7, 0x47, 0x2, 0x2, 0x84, 0x86, 0x5, 0x12, 0xa, 0x2, 0x85, 
    0x84, 0x3, 0x2, 0x2, 0x2, 0x86, 0x89, 0x3, 0x2, 0x2, 0x2, 0x87, 0x85, 
    0x3, 0x2, 0x2, 0x2, 0x87, 0x88, 0x3, 0x2, 0x2, 0x2, 0x88, 0x8a, 0x3, 
    0x2, 0x2, 0x2, 0x89, 0x87, 0x3, 0x2, 0x2, 0x2, 0x8a, 0x8b, 0x7, 0x7, 
    0x2, 0x2, 0x8b, 0xb, 0x3, 0x2, 0x2, 0x2, 0x8c, 0x9f, 0x7, 0x8, 0x2, 
    0x2, 0x8d, 0x8f, 0x7, 0x9, 0x2, 0x2, 0x8e, 0x8d, 0x3, 0x2, 0x2, 0x2, 
    0x8e, 0x8f, 0x3, 0x2, 0x2, 0x2, 0x8f, 0x96, 0x3, 0x2, 0x2, 0x2, 0x90, 
    0x97, 0x7, 0xa, 0x2, 0x2, 0x91, 0x97, 0x7, 0xb, 0x2, 0x2, 0x92, 0x97, 
    0x7, 0xc, 0x2, 0x2, 0x93, 0x97, 0x7, 0xd, 0x2, 0x2, 0x94, 0x95, 0x7, 
    0xd, 0x2, 0x2, 0x95, 0x97, 0x7, 0xd, 0x2, 0x2, 0x96, 0x90, 0x3, 0x2, 
    0x2, 0x2, 0x96, 0x91, 0x3, 0x2, 0x2, 0x2, 0x96, 0x92, 0x3, 0x2, 0x2, 
    0x2, 0x96, 0x93, 0x3, 0x2, 0x2, 0x2, 0x96, 0x94, 0x3, 0x2, 0x2, 0x2, 
    0x97, 0x9f, 0x3, 0x2, 0x2, 0x2, 0x98, 0x9f, 0x7, 0xe, 0x2, 0x2, 0x99, 
    0x9b, 0x7, 0xd, 0x2, 0x2, 0x9a, 0x99, 0x3, 0x2, 0x2, 0x2, 0x9a, 0x9b, 
    0x3, 0x2, 0x2, 0x2, 0x9b, 0x9c, 0x3, 0x2, 0x2, 0x2, 0x9c, 0x9f, 0x7, 
    0xf, 0x2, 0x2, 0x9d, 0x9f, 0x7, 0x10, 0x2, 0x2, 0x9e, 0x8c, 0x3, 0x2, 
    0x2, 0x2, 0x9e, 0x8e, 0x3, 0x2, 0x2, 0x2, 0x9e, 0x98, 0x3, 0x2, 0x2, 
    0x2, 0x9e, 0x9a, 0x3, 0x2, 0x2, 0x2, 0x9e, 0x9d, 0x3, 0x2, 0x2, 0x2, 
    0x9f, 0xd, 0x3, 0x2, 0x2, 0x2, 0xa0, 0xa3, 0x5, 0x6, 0x4, 0x2, 0xa1, 
    0xa3, 0x5, 0xc, 0x7, 0x2, 0xa2, 0xa0, 0x3, 0x2, 0x2, 0x2, 0xa2, 0xa1, 
    0x3, 0x2, 0x2, 0x2, 0xa3, 0xa7, 0x3, 0x2, 0x2, 0x2, 0xa4, 0xa6, 0x5, 
    0x10, 0x9, 0x2, 0xa5, 0xa4, 0x3, 0x2, 0x2, 0x2, 0xa6, 0xa9, 0x3, 0x2, 
    0x2, 0x2, 0xa7, 0xa5, 0x3, 0x2, 0x2, 0x2, 0xa7, 0xa8, 0x3, 0x2, 0x2, 
    0x2, 0xa8, 0xf, 0x3, 0x2, 0x2, 0x2, 0xa9, 0xa7, 0x3, 0x2, 0x2, 0x2, 
    0xaa, 0xab, 0x7, 0x11, 0x2, 0x2, 0xab, 0x11, 0x3, 0x2, 0x2, 0x2, 0xac, 
    0xad, 0x7, 0x12, 0x2, 0x2, 0xad, 0xae, 0x7, 0x42, 0x2, 0x2, 0xae, 0xaf, 
    0x7, 0x13, 0x2, 0x2, 0xaf, 0x13, 0x3, 0x2, 0x2, 0x2, 0xb0, 0xb6, 0x7, 
    0x14, 0x2, 0x2, 0xb1, 0xb2, 0x5, 0x16, 0xc, 0x2, 0xb2, 0xb3, 0x7, 0x7, 
    0x2, 0x2, 0xb3, 0xb5, 0x3, 0x2, 0x2, 0x2, 0xb4, 0xb1, 0x3, 0x2, 0x2, 
    0x2, 0xb5, 0xb8, 0x3, 0x2, 0x2, 0x2, 0xb6, 0xb4, 0x3, 0x2, 0x2, 0x2, 
    0xb6, 0xb7, 0x3, 0x2, 0x2, 0x2, 0xb7, 0x15, 0x3, 0x2, 0x2, 0x2, 0xb8, 
    0xb6, 0x3, 0x2, 0x2, 0x2, 0xb9, 0xbb, 0x8, 0xc, 0x1, 0x2, 0xba, 0xbc, 
    0x7, 0x15, 0x2, 0x2, 0xbb, 0xba, 0x3, 0x2, 0x2, 0x2, 0xbb, 0xbc, 0x3, 
    0x2, 0x2, 0x2, 0xbc, 0xbd, 0x3, 0x2, 0x2, 0x2, 0xbd, 0xc3, 0x5, 0x18, 
    0xd, 0x2, 0xbe, 0xbf, 0x5, 0x30, 0x19, 0x2, 0xbf, 0xc0, 0x7, 0x18, 0x2, 
    0x2, 0xc0, 0xc1, 0x5, 0x16, 0xc, 0x3, 0xc1, 0xc3, 0x3, 0x2, 0x2, 0x2, 
    0xc2, 0xb9, 0x3, 0x2, 0x2, 0x2, 0xc2, 0xbe, 0x3, 0x2, 0x2, 0x2, 0xc3, 
    0xcc, 0x3, 0x2, 0x2, 0x2, 0xc4, 0xc5, 0xc, 0x5, 0x2, 0x2, 0xc5, 0xc6, 
    0x7, 0x16, 0x2, 0x2, 0xc6, 0xcb, 0x5, 0x16, 0xc, 0x6, 0xc7, 0xc8, 0xc, 
    0x4, 0x2, 0x2, 0xc8, 0xc9, 0x7, 0x17, 0x2, 0x2, 0xc9, 0xcb, 0x5, 0x16, 
    0xc, 0x5, 0xca, 0xc4, 0x3, 0x2, 0x2, 0x2, 0xca, 0xc7, 0x3, 0x2, 0x2, 
    0x2, 0xcb, 0xce, 0x3, 0x2, 0x2, 0x2, 0xcc, 0xca, 0x3, 0x2, 0x2, 0x2, 
    0xcc, 0xcd, 0x3, 0x2, 0x2, 0x2, 0xcd, 0x17, 0x3, 0x2, 0x2, 0x2, 0xce, 
    0xcc, 0x3, 0x2, 0x2, 0x2, 0xcf, 0xd0, 0x5, 0x1a, 0xe, 0x2, 0xd0, 0x19, 
    0x3, 0x2, 0x2, 0x2, 0xd1, 0xd2, 0x7, 0x47, 0x2, 0x2, 0xd2, 0xd4, 0x7, 
    0x12, 0x2, 0x2, 0xd3, 0xd5, 0x5, 0x1c, 0xf, 0x2, 0xd4, 0xd3, 0x3, 0x2, 
    0x2, 0x2, 0xd4, 0xd5, 0x3, 0x2, 0x2, 0x2, 0xd5, 0xd6, 0x3, 0x2, 0x2, 
    0x2, 0xd6, 0xd7, 0x7, 0x13, 0x2, 0x2, 0xd7, 0x1b, 0x3, 0x2, 0x2, 0x2, 
    0xd8, 0xdd, 0x5, 0x1e, 0x10, 0x2, 0xd9, 0xda, 0x7, 0x16, 0x2, 0x2, 0xda, 
    0xdc, 0x5, 0x1e, 0x10, 0x2, 0xdb, 0xd9, 0x3, 0x2, 0x2, 0x2, 0xdc, 0xdf, 
    0x3, 0x2, 0x2, 0x2, 0xdd, 0xdb, 0x3, 0x2, 0x2, 0x2, 0xdd, 0xde, 0x3, 
    0x2, 0x2, 0x2, 0xde, 0x1d, 0x3, 0x2, 0x2, 0x2, 0xdf, 0xdd, 0x3, 0x2, 
    0x2, 0x2, 0xe0, 0xe4, 0x5, 0x20, 0x11, 0x2, 0xe1, 0xe4, 0x7, 0x19, 0x2, 
    0x2, 0xe2, 0xe4, 0x7, 0x1a, 0x2, 0x2, 0xe3, 0xe0, 0x3, 0x2, 0x2, 0x2, 
    0xe3, 0xe1, 0x3, 0x2, 0x2, 0x2, 0xe3, 0xe2, 0x3, 0x2, 0x2, 0x2, 0xe4, 
    0x1f, 0x3, 0x2, 0x2, 0x2, 0xe5, 0xe6, 0x5, 0x22, 0x12, 0x2, 0xe6, 0x21, 
    0x3, 0x2, 0x2, 0x2, 0xe7, 0xeb, 0x5, 0x24, 0x13, 0x2, 0xe8, 0xeb, 0x5, 
    0x26, 0x14, 0x2, 0xe9, 0xeb, 0x5, 0x28, 0x15, 0x2, 0xea, 0xe7, 0x3, 
    0x2, 0x2, 0x2, 0xea, 0xe8, 0x3, 0x2, 0x2, 0x2, 0xea, 0xe9, 0x3, 0x2, 
    0x2, 0x2, 0xeb, 0x23, 0x3, 0x2, 0x2, 0x2, 0xec, 0xf3, 0x7, 0x42, 0x2, 
    0x2, 0xed, 0xee, 0x7, 0x42, 0x2, 0x2, 0xee, 0xef, 0x7, 0x1b, 0x2, 0x2, 
    0xef, 0xf3, 0x7, 0x42, 0x2, 0x2, 0xf0, 0xf3, 0x7, 0x45, 0x2, 0x2, 0xf1, 
    0xf3, 0x7, 0x46, 0x2, 0x2, 0xf2, 0xec, 0x3, 0x2, 0x2, 0x2, 0xf2, 0xed, 
    0x3, 0x2, 0x2, 0x2, 0xf2, 0xf0, 0x3, 0x2, 0x2, 0x2, 0xf2, 0xf1, 0x3, 
    0x2, 0x2, 0x2, 0xf3, 0x25, 0x3, 0x2, 0x2, 0x2, 0xf4, 0xfe, 0x7, 0x47, 
    0x2, 0x2, 0xf5, 0xf6, 0x7, 0x11, 0x2, 0x2, 0xf6, 0xfe, 0x7, 0x47, 0x2, 
    0x2, 0xf7, 0xf8, 0x7, 0x47, 0x2, 0x2, 0xf8, 0xf9, 0x7, 0x1c, 0x2, 0x2, 
    0xf9, 0xfe, 0x7, 0x47, 0x2, 0x2, 0xfa, 0xfb, 0x7, 0x47, 0x2, 0x2, 0xfb, 
    0xfc, 0x7, 0x1d, 0x2, 0x2, 0xfc, 0xfe, 0x7, 0x47, 0x2, 0x2, 0xfd, 0xf4, 
    0x3, 0x2, 0x2, 0x2, 0xfd, 0xf5, 0x3, 0x2, 0x2, 0x2, 0xfd, 0xf7, 0x3, 
    0x2, 0x2, 0x2, 0xfd, 0xfa, 0x3, 0x2, 0x2, 0x2, 0xfe, 0x27, 0x3, 0x2, 
    0x2, 0x2, 0xff, 0x100, 0x7, 0x1e, 0x2, 0x2, 0x100, 0x101, 0x7, 0x12, 
    0x2, 0x2, 0x101, 0x102, 0x5, 0x26, 0x14, 0x2, 0x102, 0x103, 0x7, 0x16, 
    0x2, 0x2, 0x103, 0x104, 0x5, 0xe, 0x8, 0x2, 0x104, 0x105, 0x7, 0x13, 
    0x2, 0x2, 0x105, 0x119, 0x3, 0x2, 0x2, 0x2, 0x106, 0x107, 0x7, 0x1f, 
    0x2, 0x2, 0x107, 0x108, 0x7, 0x12, 0x2, 0x2, 0x108, 0x109, 0x7, 0x47, 
    0x2, 0x2, 0x109, 0x119, 0x7, 0x13, 0x2, 0x2, 0x10a, 0x10b, 0x7, 0x20, 
    0x2, 0x2, 0x10b, 0x10c, 0x7, 0x12, 0x2, 0x2, 0x10c, 0x10d, 0x7, 0x47, 
    0x2, 0x2, 0x10d, 0x119, 0x7, 0x13, 0x2, 0x2, 0x10e, 0x10f, 0x7, 0x21, 
    0x2, 0x2, 0x10f, 0x110, 0x7, 0x12, 0x2, 0x2, 0x110, 0x111, 0x5, 0x26, 
    0x14, 0x2, 0x111, 0x112, 0x7, 0x13, 0x2, 0x2, 0x112, 0x119, 0x3, 0x2, 
    0x2, 0x2, 0x113, 0x114, 0x7, 0x22, 0x2, 0x2, 0x114, 0x115, 0x7, 0x12, 
    0x2, 0x2, 0x115, 0x116, 0x5, 0x26, 0x14, 0x2, 0x116, 0x117, 0x7, 0x13, 
    0x2, 0x2, 0x117, 0x119, 0x3, 0x2, 0x2, 0x2, 0x118, 0xff, 0x3, 0x2, 0x2, 
    0x2, 0x118, 0x106, 0x3, 0x2, 0x2, 0x2, 0x118, 0x10a, 0x3, 0x2, 0x2, 
    0x2, 0x118, 0x10e, 0x3, 0x2, 0x2, 0x2, 0x118, 0x113, 0x3, 0x2, 0x2, 
    0x2, 0x119, 0x29, 0x3, 0x2, 0x2, 0x2, 0x11a, 0x11e, 0x7, 0x23, 0x2, 
    0x2, 0x11b, 0x11c, 0x5, 0x2c, 0x17, 0x2, 0x11c, 0x11d, 0x7, 0x7, 0x2, 
    0x2, 0x11d, 0x11f, 0x3, 0x2, 0x2, 0x2, 0x11e, 0x11b, 0x3, 0x2, 0x2, 
    0x2, 0x11f, 0x120, 0x3, 0x2, 0x2, 0x2, 0x120, 0x11e, 0x3, 0x2, 0x2, 
    0x2, 0x120, 0x121, 0x3, 0x2, 0x2, 0x2, 0x121, 0x2b, 0x3, 0x2, 0x2, 0x2, 
    0x122, 0x125, 0x5, 0x1a, 0xe, 0x2, 0x123, 0x124, 0x7, 0x24, 0x2, 0x2, 
    0x124, 0x126, 0x7, 0x47, 0x2, 0x2, 0x125, 0x123, 0x3, 0x2, 0x2, 0x2, 
    0x125, 0x126, 0x3, 0x2, 0x2, 0x2, 0x126, 0x2d, 0x3, 0x2, 0x2, 0x2, 0x127, 
    0x12b, 0x7, 0x25, 0x2, 0x2, 0x128, 0x129, 0x5, 0x30, 0x19, 0x2, 0x129, 
    0x12a, 0x7, 0x7, 0x2, 0x2, 0x12a, 0x12c, 0x3, 0x2, 0x2, 0x2, 0x12b, 
    0x128, 0x3, 0x2, 0x2, 0x2, 0x12c, 0x12d, 0x3, 0x2, 0x2, 0x2, 0x12d, 
    0x12b, 0x3, 0x2, 0x2, 0x2, 0x12d, 0x12e, 0x3, 0x2, 0x2, 0x2, 0x12e, 
    0x2f, 0x3, 0x2, 0x2, 0x2, 0x12f, 0x138, 0x8, 0x19, 0x1, 0x2, 0x130, 
    0x131, 0x7, 0x26, 0x2, 0x2, 0x131, 0x132, 0x5, 0x30, 0x19, 0x2, 0x132, 
    0x133, 0x7, 0x27, 0x2, 0x2, 0x133, 0x138, 0x3, 0x2, 0x2, 0x2, 0x134, 
    0x138, 0x5, 0x34, 0x1b, 0x2, 0x135, 0x136, 0x7, 0x15, 0x2, 0x2, 0x136, 
    0x138, 0x5, 0x30, 0x19, 0xb, 0x137, 0x12f, 0x3, 0x2, 0x2, 0x2, 0x137, 
    0x130, 0x3, 0x2, 0x2, 0x2, 0x137, 0x134, 0x3, 0x2, 0x2, 0x2, 0x137, 
    0x135, 0x3, 0x2, 0x2, 0x2, 0x138, 0x15d, 0x3, 0x2, 0x2, 0x2, 0x139, 
    0x13c, 0xc, 0xa, 0x2, 0x2, 0x13a, 0x13d, 0x7, 0x11, 0x2, 0x2, 0x13b, 
    0x13d, 0x7, 0x28, 0x2, 0x2, 0x13c, 0x13a, 0x3, 0x2, 0x2, 0x2, 0x13c, 
    0x13b, 0x3, 0x2, 0x2, 0x2, 0x13d, 0x13e, 0x3, 0x2, 0x2, 0x2, 0x13e, 
    0x15c, 0x5, 0x30, 0x19, 0xb, 0x13f, 0x140, 0xc, 0x9, 0x2, 0x2, 0x140, 
    0x141, 0x7, 0x29, 0x2, 0x2, 0x141, 0x15c, 0x5, 0x30, 0x19, 0xa, 0x142, 
    0x145, 0xc, 0x8, 0x2, 0x2, 0x143, 0x146, 0x7, 0x2a, 0x2, 0x2, 0x144, 
    0x146, 0x7, 0x2b, 0x2, 0x2, 0x145, 0x143, 0x3, 0x2, 0x2, 0x2, 0x145, 
    0x144, 0x3, 0x2, 0x2, 0x2, 0x146, 0x147, 0x3, 0x2, 0x2, 0x2, 0x147, 
    0x15c, 0x5, 0x30, 0x19, 0x9, 0x148, 0x149, 0xc, 0x7, 0x2, 0x2, 0x149, 
    0x14a, 0x5, 0x32, 0x1a, 0x2, 0x14a, 0x14b, 0x5, 0x30, 0x19, 0x8, 0x14b, 
    0x15c, 0x3, 0x2, 0x2, 0x2, 0x14c, 0x14f, 0xc, 0x6, 0x2, 0x2, 0x14d, 
    0x150, 0x7, 0x2c, 0x2, 0x2, 0x14e, 0x150, 0x7, 0x2d, 0x2, 0x2, 0x14f, 
    0x14d, 0x3, 0x2, 0x2, 0x2, 0x14f, 0x14e, 0x3, 0x2, 0x2, 0x2, 0x150, 
    0x151, 0x3, 0x2, 0x2, 0x2, 0x151, 0x15c, 0x5, 0x30, 0x19, 0x7, 0x152, 
    0x153, 0xc, 0x5, 0x2, 0x2, 0x153, 0x154, 0x7, 0x2e, 0x2, 0x2, 0x154, 
    0x15c, 0x5, 0x30, 0x19, 0x6, 0x155, 0x156, 0xc, 0x4, 0x2, 0x2, 0x156, 
    0x157, 0x7, 0x17, 0x2, 0x2, 0x157, 0x15c, 0x5, 0x30, 0x19, 0x5, 0x158, 
    0x159, 0xc, 0x3, 0x2, 0x2, 0x159, 0x15a, 0x7, 0x18, 0x2, 0x2, 0x15a, 
    0x15c, 0x5, 0x30, 0x19, 0x3, 0x15b, 0x139, 0x3, 0x2, 0x2, 0x2, 0x15b, 
    0x13f, 0x3, 0x2, 0x2, 0x2, 0x15b, 0x142, 0x3, 0x2, 0x2, 0x2, 0x15b, 
    0x148, 0x3, 0x2, 0x2, 0x2, 0x15b, 0x14c, 0x3, 0x2, 0x2, 0x2, 0x15b, 
    0x152, 0x3, 0x2, 0x2, 0x2, 0x15b, 0x155, 0x3, 0x2, 0x2, 0x2, 0x15b, 
    0x158, 0x3, 0x2, 0x2, 0x2, 0x15c, 0x15f, 0x3, 0x2, 0x2, 0x2, 0x15d, 
    0x15b, 0x3, 0x2, 0x2, 0x2, 0x15d, 0x15e, 0x3, 0x2, 0x2, 0x2, 0x15e, 
    0x31, 0x3, 0x2, 0x2, 0x2, 0x15f, 0x15d, 0x3, 0x2, 0x2, 0x2, 0x160, 0x165, 
    0x7, 0x2f, 0x2, 0x2, 0x161, 0x165, 0x7, 0x30, 0x2, 0x2, 0x162, 0x165, 
    0x7, 0x31, 0x2, 0x2, 0x163, 0x165, 0x7, 0x32, 0x2, 0x2, 0x164, 0x160, 
    0x3, 0x2, 0x2, 0x2, 0x164, 0x161, 0x3, 0x2, 0x2, 0x2, 0x164, 0x162, 
    0x3, 0x2, 0x2, 0x2, 0x164, 0x163, 0x3, 0x2, 0x2, 0x2, 0x165, 0x33, 0x3, 
    0x2, 0x2, 0x2, 0x166, 0x167, 0x5, 0x36, 0x1c, 0x2, 0x167, 0x168, 0x7, 
    0x33, 0x2, 0x2, 0x168, 0x169, 0x7, 0x34, 0x2, 0x2, 0x169, 0x16a, 0x5, 
    0x38, 0x1d, 0x2, 0x16a, 0x16b, 0x7, 0x35, 0x2, 0x2, 0x16b, 0x16e, 0x3, 
    0x2, 0x2, 0x2, 0x16c, 0x16e, 0x5, 0x22, 0x12, 0x2, 0x16d, 0x166, 0x3, 
    0x2, 0x2, 0x2, 0x16d, 0x16c, 0x3, 0x2, 0x2, 0x2, 0x16e, 0x35, 0x3, 0x2, 
    0x2, 0x2, 0x16f, 0x170, 0x7, 0x36, 0x2, 0x2, 0x170, 0x171, 0x7, 0x26, 
    0x2, 0x2, 0x171, 0x172, 0x5, 0x20, 0x11, 0x2, 0x172, 0x173, 0x7, 0x27, 
    0x2, 0x2, 0x173, 0x176, 0x3, 0x2, 0x2, 0x2, 0x174, 0x176, 0x5, 0x20, 
    0x11, 0x2, 0x175, 0x16f, 0x3, 0x2, 0x2, 0x2, 0x175, 0x174, 0x3, 0x2, 
    0x2, 0x2, 0x176, 0x37, 0x3, 0x2, 0x2, 0x2, 0x177, 0x17f, 0x5, 0x24, 
    0x13, 0x2, 0x178, 0x17b, 0x7, 0x16, 0x2, 0x2, 0x179, 0x17c, 0x5, 0x24, 
    0x13, 0x2, 0x17a, 0x17c, 0x7, 0x37, 0x2, 0x2, 0x17b, 0x179, 0x3, 0x2, 
    0x2, 0x2, 0x17b, 0x17a, 0x3, 0x2, 0x2, 0x2, 0x17c, 0x17e, 0x3, 0x2, 
    0x2, 0x2, 0x17d, 0x178, 0x3, 0x2, 0x2, 0x2, 0x17e, 0x181, 0x3, 0x2, 
    0x2, 0x2, 0x17f, 0x17d, 0x3, 0x2, 0x2, 0x2, 0x17f, 0x180, 0x3, 0x2, 
    0x2, 0x2, 0x180, 0x39, 0x3, 0x2, 0x2, 0x2, 0x181, 0x17f, 0x3, 0x2, 0x2, 
    0x2, 0x182, 0x184, 0x7, 0x38, 0x2, 0x2, 0x183, 0x185, 0x5, 0x3c, 0x1f, 
    0x2, 0x184, 0x183, 0x3, 0x2, 0x2, 0x2, 0x185, 0x186, 0x3, 0x2, 0x2, 
    0x2, 0x186, 0x184, 0x3, 0x2, 0x2, 0x2, 0x186, 0x187, 0x3, 0x2, 0x2, 
    0x2, 0x187, 0x3b, 0x3, 0x2, 0x2, 0x2, 0x188, 0x189, 0x7, 0x47, 0x2, 
    0x2, 0x189, 0x18c, 0x7, 0x39, 0x2, 0x2, 0x18a, 0x18b, 0x7, 0x47, 0x2, 
    0x2, 0x18b, 0x18d, 0x7, 0x3a, 0x2, 0x2, 0x18c, 0x18a, 0x3, 0x2, 0x2, 
    0x2, 0x18c, 0x18d, 0x3, 0x2, 0x2, 0x2, 0x18d, 0x18e, 0x3, 0x2, 0x2, 
    0x2, 0x18e, 0x18f, 0x7, 0x47, 0x2, 0x2, 0x18f, 0x191, 0x7, 0x26, 0x2, 
    0x2, 0x190, 0x192, 0x5, 0x3e, 0x20, 0x2, 0x191, 0x190, 0x3, 0x2, 0x2, 
    0x2, 0x191, 0x192, 0x3, 0x2, 0x2, 0x2, 0x192, 0x193, 0x3, 0x2, 0x2, 
    0x2, 0x193, 0x194, 0x7, 0x27, 0x2, 0x2, 0x194, 0x195, 0x7, 0x7, 0x2, 
    0x2, 0x195, 0x3d, 0x3, 0x2, 0x2, 0x2, 0x196, 0x19b, 0x5, 0x40, 0x21, 
    0x2, 0x197, 0x198, 0x7, 0x16, 0x2, 0x2, 0x198, 0x19a, 0x5, 0x40, 0x21, 
    0x2, 0x199, 0x197, 0x3, 0x2, 0x2, 0x2, 0x19a, 0x19d, 0x3, 0x2, 0x2, 
    0x2, 0x19b, 0x199, 0x3, 0x2, 0x2, 0x2, 0x19b, 0x19c, 0x3, 0x2, 0x2, 
    0x2, 0x19c, 0x3f, 0x3, 0x2, 0x2, 0x2, 0x19d, 0x19b, 0x3, 0x2, 0x2, 0x2, 
    0x19e, 0x1a2, 0x5, 0x26, 0x14, 0x2, 0x19f, 0x1a2, 0x7, 0x19, 0x2, 0x2, 
    0x1a0, 0x1a2, 0x7, 0x1a, 0x2, 0x2, 0x1a1, 0x19e, 0x3, 0x2, 0x2, 0x2, 
    0x1a1, 0x19f, 0x3, 0x2, 0x2, 0x2, 0x1a1, 0x1a0, 0x3, 0x2, 0x2, 0x2, 
    0x1a2, 0x41, 0x3, 0x2, 0x2, 0x2, 0x1a3, 0x1a4, 0x7, 0x3b, 0x2, 0x2, 
    0x1a4, 0x1a5, 0x5, 0x44, 0x23, 0x2, 0x1a5, 0x43, 0x3, 0x2, 0x2, 0x2, 
    0x1a6, 0x1ab, 0x5, 0x46, 0x24, 0x2, 0x1a7, 0x1a8, 0x7, 0x16, 0x2, 0x2, 
    0x1a8, 0x1aa, 0x5, 0x46, 0x24, 0x2, 0x1a9, 0x1a7, 0x3, 0x2, 0x2, 0x2, 
    0x1aa, 0x1ad, 0x3, 0x2, 0x2, 0x2, 0x1ab, 0x1a9, 0x3, 0x2, 0x2, 0x2, 
    0x1ab, 0x1ac, 0x3, 0x2, 0x2, 0x2, 0x1ac, 0x45, 0x3, 0x2, 0x2, 0x2, 0x1ad, 
    0x1ab, 0x3, 0x2, 0x2, 0x2, 0x1ae, 0x1b3, 0x5, 0x48, 0x25, 0x2, 0x1af, 
    0x1b0, 0x7, 0x3c, 0x2, 0x2, 0x1b0, 0x1b2, 0x5, 0x48, 0x25, 0x2, 0x1b1, 
    0x1af, 0x3, 0x2, 0x2, 0x2, 0x1b2, 0x1b5, 0x3, 0x2, 0x2, 0x2, 0x1b3, 
    0x1b1, 0x3, 0x2, 0x2, 0x2, 0x1b3, 0x1b4, 0x3, 0x2, 0x2, 0x2, 0x1b4, 
    0x47, 0x3, 0x2, 0x2, 0x2, 0x1b5, 0x1b3, 0x3, 0x2, 0x2, 0x2, 0x1b6, 0x1c6, 
    0x5, 0x4a, 0x26, 0x2, 0x1b7, 0x1b8, 0x7, 0x3d, 0x2, 0x2, 0x1b8, 0x1ba, 
    0x5, 0x4a, 0x26, 0x2, 0x1b9, 0x1b7, 0x3, 0x2, 0x2, 0x2, 0x1ba, 0x1bb, 
    0x3, 0x2, 0x2, 0x2, 0x1bb, 0x1b9, 0x3, 0x2, 0x2, 0x2, 0x1bb, 0x1bc, 
    0x3, 0x2, 0x2, 0x2, 0x1bc, 0x1c4, 0x3, 0x2, 0x2, 0x2, 0x1bd, 0x1bf, 
    0x7, 0x42, 0x2, 0x2, 0x1be, 0x1bd, 0x3, 0x2, 0x2, 0x2, 0x1be, 0x1bf, 
    0x3, 0x2, 0x2, 0x2, 0x1bf, 0x1c0, 0x3, 0x2, 0x2, 0x2, 0x1c0, 0x1c2, 
    0x7, 0x3e, 0x2, 0x2, 0x1c1, 0x1c3, 0x7, 0x42, 0x2, 0x2, 0x1c2, 0x1c1, 
    0x3, 0x2, 0x2, 0x2, 0x1c2, 0x1c3, 0x3, 0x2, 0x2, 0x2, 0x1c3, 0x1c5, 
    0x3, 0x2, 0x2, 0x2, 0x1c4, 0x1be, 0x3, 0x2, 0x2, 0x2, 0x1c4, 0x1c5, 
    0x3, 0x2, 0x2, 0x2, 0x1c5, 0x1c7, 0x3, 0x2, 0x2, 0x2, 0x1c6, 0x1b9, 
    0x3, 0x2, 0x2, 0x2, 0x1c6, 0x1c7, 0x3, 0x2, 0x2, 0x2, 0x1c7, 0x49, 0x3, 
    0x2, 0x2, 0x2, 0x1c8, 0x1ca, 0x7, 0x47, 0x2, 0x2, 0x1c9, 0x1cb, 0x9, 
    0x2, 0x2, 0x2, 0x1ca, 0x1c9, 0x3, 0x2, 0x2, 0x2, 0x1ca, 0x1cb, 0x3, 
    0x2, 0x2, 0x2, 0x1cb, 0x1d3, 0x3, 0x2, 0x2, 0x2, 0x1cc, 0x1cd, 0x7, 
    0x26, 0x2, 0x2, 0x1cd, 0x1ce, 0x5, 0x44, 0x23, 0x2, 0x1ce, 0x1d0, 0x7, 
    0x27, 0x2, 0x2, 0x1cf, 0x1d1, 0x9, 0x2, 0x2, 0x2, 0x1d0, 0x1cf, 0x3, 
    0x2, 0x2, 0x2, 0x1d0, 0x1d1, 0x3, 0x2, 0x2, 0x2, 0x1d1, 0x1d3, 0x3, 
    0x2, 0x2, 0x2, 0x1d2, 0x1c8, 0x3, 0x2, 0x2, 0x2, 0x1d2, 0x1cc, 0x3, 
    0x2, 0x2, 0x2, 0x1d3, 0x4b, 0x3, 0x2, 0x2, 0x2, 0x1d4, 0x1d6, 0x7, 0x40, 
    0x2, 0x2, 0x1d5, 0x1d7, 0x5, 0x4e, 0x28, 0x2, 0x1d6, 0x1d5, 0x3, 0x2, 
    0x2, 0x2, 0x1d7, 0x1d8, 0x3, 0x2, 0x2, 0x2, 0x1d8, 0x1d6, 0x3, 0x2, 
    0x2, 0x2, 0x1d8, 0x1d9, 0x3, 0x2, 0x2, 0x2, 0x1d9, 0x4d, 0x3, 0x2, 0x2, 
    0x2, 0x1da, 0x1db, 0x5, 0x2c, 0x17, 0x2, 0x1db, 0x4f, 0x3, 0x2, 0x2, 
    0x2, 0x1dc, 0x1de, 0x7, 0x41, 0x2, 0x2, 0x1dd, 0x1df, 0x5, 0x52, 0x2a, 
    0x2, 0x1de, 0x1dd, 0x3, 0x2, 0x2, 0x2, 0x1df, 0x1e0, 0x3, 0x2, 0x2, 
    0x2, 0x1e0, 0x1de, 0x3, 0x2, 0x2, 0x2, 0x1e0, 0x1e1, 0x3, 0x2, 0x2, 
    0x2, 0x1e1, 0x51, 0x3, 0x2, 0x2, 0x2, 0x1e2, 0x1e3, 0x5, 0x54, 0x2b, 
    0x2, 0x1e3, 0x1e4, 0x7, 0x18, 0x2, 0x2, 0x1e4, 0x1e5, 0x7, 0x47, 0x2, 
    0x2, 0x1e5, 0x1e6, 0x3, 0x2, 0x2, 0x2, 0x1e6, 0x1e7, 0x7, 0x7, 0x2, 
    0x2, 0x1e7, 0x53, 0x3, 0x2, 0x2, 0x2, 0x1e8, 0x1e9, 0x5, 0x6, 0x4, 0x2, 
    0x1e9, 0x1eb, 0x7, 0x26, 0x2, 0x2, 0x1ea, 0x1ec, 0x5, 0x56, 0x2c, 0x2, 
    0x1eb, 0x1ea, 0x3, 0x2, 0x2, 0x2, 0x1eb, 0x1ec, 0x3, 0x2, 0x2, 0x2, 
    0x1ec, 0x1ed, 0x3, 0x2, 0x2, 0x2, 0x1ed, 0x1ee, 0x7, 0x27, 0x2, 0x2, 
    0x1ee, 0x55, 0x3, 0x2, 0x2, 0x2, 0x1ef, 0x1f4, 0x5, 0xe, 0x8, 0x2, 0x1f0, 
    0x1f1, 0x7, 0x16, 0x2, 0x2, 0x1f1, 0x1f3, 0x5, 0xe, 0x8, 0x2, 0x1f2, 
    0x1f0, 0x3, 0x2, 0x2, 0x2, 0x1f3, 0x1f6, 0x3, 0x2, 0x2, 0x2, 0x1f4, 
    0x1f2, 0x3, 0x2, 0x2, 0x2, 0x1f4, 0x1f5, 0x3, 0x2, 0x2, 0x2, 0x1f5, 
    0x57, 0x3, 0x2, 0x2, 0x2, 0x1f6, 0x1f4, 0x3, 0x2, 0x2, 0x2, 0x3e, 0x5b, 
    0x60, 0x63, 0x66, 0x69, 0x75, 0x7c, 0x80, 0x87, 0x8e, 0x96, 0x9a, 0x9e, 
    0xa2, 0xa7, 0xb6, 0xbb, 0xc2, 0xca, 0xcc, 0xd4, 0xdd, 0xe3, 0xea, 0xf2, 
    0xfd, 0x118, 0x120, 0x125, 0x12d, 0x137, 0x13c, 0x145, 0x14f, 0x15b, 
    0x15d, 0x164, 0x16d, 0x175, 0x17b, 0x17f, 0x186, 0x18c, 0x191, 0x19b, 
    0x1a1, 0x1ab, 0x1b3, 0x1bb, 0x1be, 0x1c2, 0x1c4, 0x1c6, 0x1ca, 0x1d0, 
    0x1d2, 0x1d8, 0x1e0, 0x1eb, 0x1f4, 
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
