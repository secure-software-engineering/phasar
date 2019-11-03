#include <FormatConverter/FactoryObject.h>
#include <FormatConverter/Predicate.h>
#include <FormatConverter/PredicateConverter.h>
#include <isotream>
#include <list>

namespace CCPP {

/*PredicateConverter::PredicateConverter(
    CrySLParser::PredContext *predicateCtxObj) {
  this->predCtx = predicateCtxObj;
}*/

PredicateConverter::PredicateConverter(CrySLParser::EnsPredContext *ensCtxObj,
                                       CrySLParser::ObjectsContext *objCtx,
                                       const std::string &specName)
    : specName(specName) {
  this->ensCtx = ensCtxObj;
  this->objCtx = objCtx;
}

PredicateConverter::PredicateConverter(CrySLParser::ReqPredContext *reqCtxObj,
                                       CrySLParser::ObjectsContext *objCtx,
                                       const std::string &specName)
    : specName(specName) {
  this->reqCtx = reqCtxObj;
  this->objCtx = objCtx;
}

std::vector<Predicate> PredicateConverter::formatConverter() {
  Predicate predicateObj;
  std::vector<std::unique_ptr<DefinedObject>> params;
  std::vector<Predicate> predicates;
  FactoryObject facObj(objCtx, specName);
  std::list<reqCtx *> reqList;
  
  auto init = this->reqCtx->reqPred();
  reqList.push_back(init);                                                 //need to check for correct initialization for this list to work
  for (auto reqPreds : reqList) { // this->reqCtx->reqPred())
    predicateObj.setFunctionName(reqPreds->reqPredLit()->pred()->name);
    reqList.insert(reqList.end(), init.begin(), init.end());
    // predicateObj.setFunctionName(reqPreds->reqPredLit);
    if (reqPreds->reqPredLit()) {
      for (auto param : reqPreds->reqPredLit()->pred()->suParList()->suPar()) {
        // params.emplace_back(param->value->literalExpr()->memberAccess());
        if (param->value->literalExpr()
                ->memberAccess()) { // creates unique pointer of type
                                    // DefinedObject using factory
          params.push_back(std::move(
              facObj.createObject( //  and inserts it in vector of params
                  param->value->literalExpr()->memberAccess())));
        } else { // we need to handle this ptr separately b/c in memberAccess it
                 // is null
          params.push_back(std::move(facObj.createThisObject()));
        }
      }
    }

    else {
      auto additionalPreds = reqPreds->reqPred();
      reqList.insert(reqList.end(), additionalPreds.begin(),
                     additionalPreds.end());
    }

    /* if (reqPreds->reqPred()) {
       for (auto param : reqPreds->reqPred()) {
         params.emplace_back(param->reqPred());
       }
     }*/

    predicateObj.setParams(std::move(params));
    predicates.push_back(std::move(predicateObj));
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

bool Predicate::operator==(const Predicate &pc) const { 
	if (this->setParams == pc.params)
		return true;
    return false;
}

bool Predicate::operator==(const Predicate &pc) const {
  if (this->functionName == pc.functionName)
    return true;
  return false;
}
} // namespace CCPP