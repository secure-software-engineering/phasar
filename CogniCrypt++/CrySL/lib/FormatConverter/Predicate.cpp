#include <FormatConverter/Predicate.h>
#include <FormatConverter/PredicateConverter.h>
#include <isotream>

namespace CCPP {

/*PredicateConverter::PredicateConverter(
    CrySLParser::PredContext *predicateCtxObj) {
  this->predCtx = predicateCtxObj;
}*/

PredicateConverter::PredicateConverter(CrySLParser::EnsPredContext *ensCtxObj) {
  this->ensCtx = ensCtxObj;
}

PredicateConverter::PredicateConverter(CrySLParser::ReqPredContext *reqCtxObj) {
  this->reqCtx = reqCtxObj;
}

std::vector<Predicate> PredicateConverter::formatConverter() {
  Predicate predicateObj;
  std::vector<DefinedObject> params;
  std::vector<Predicate> predicates;
  std::initializer_list<reqCtx *> reqList;
  //std::list<reqCtx *> reqList;
  auto init = this->reqCtx->reqPred();

  for (auto reqPreds : reqList) {//this->reqCtx->reqPred()) 
    predicateObj.setFunctionName(reqPreds->reqPredLit()->pred()->name);
    reqList.insert(reqList.end(), init.begin(), init.end());
    // predicateObj.setFunctionName(reqPreds->reqPredLit);
    if (reqPreds->reqPredLit()) {
      for (auto param : reqPreds->reqPredLit()->pred()->suParList()->suPar()) {
        params.emplace_back(param->value->literalExpr()->memberAccess());
      }
    }

    else {
      auto additionalPreds = reqPreds->reqPred();
      reqList.insert(reqList.end(),additionalPreds.begin(),additionalPreds.end());
    }

    /* if (reqPreds->reqPred()) {
       for (auto param : reqPreds->reqPred()) {
         params.emplace_back(param->reqPred());
       }
     }*/

    predicateObj.setParams(params);
    predicates.push_back(predicateObj);
  }

  for (auto ensPreds : this->ensCtx->pred()) {
    predicateObj.setFunctionName(ensCtx->pred()->name);

    if (ensCtx->constr()->constr()) {
      for (auto param : ensCtx->constr()->constr()) {
        params.emplace_back(param->constr());
      }
    }

    /* if (ensCtx->constr()->cons()->literalExpr()->preDefinedPredicate()) {
       for (auto param :
     ensCtx->constr()->cons()->arrayElements()->consPred()->literalExpr()->memberAccess()){
         params.emplace_back();
           }
     }*/

    if (ensCtx->pred()->paramList()) {
      for (auto param : ensCtx->pred()) {
        params.emplace_back();
      }
    }

    predicateObj.setParams(params);
    predicates.push_back(predicateObj);
  }

  /*for (auto predicate : this->predCtx->Ident()) {
    predicateObj.setParams();
  }*/

  /*for (auto ensPred : this->ensCtx->constr()) {
    predicateObj.getFunctionName();
        if
  }*/
}

bool Predicate::operator==(const Predicate &pc) {
  if (this->functionName == pc.functionName)
    return true;
  return false;
}
} // namespace CCPP