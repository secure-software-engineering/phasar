#include <FormatConverter/Predicate.h>
#include <FormatConverter/PredicateConverter.h>
#include <isotream>

namespace CCPP {
PredicateConverter::PredicateConverter(CrySLParser::PredContext *predicateCtxObj) { //TODO
  this->predCtx = predicateCtxObj;
}

PredicateConverter::PredicateConverter(CrySLParser::EnsPredContext *ensCtxObj) { //TODO
}

PredicateConverter::PredicateConverter(CrySLParser::ReqPredContext *reqCtxObj) { //TODO
}

std::vector<Predicate> PredicateConverter::formatConverter() { //TO CHECK, If needed or not.
  Predicate predicateObj;
  std::vector<ObjectWithOutLLVM> params;
  std::vector<Predicate> predicates;

  /*for (auto prediacte : this->predCtx->Ident()) {
  }*/

  for (auto ensPred : this->ensCtx->constr()) {
    predicateObj.getFunctionName();
	if 
  }
}
bool Predicate::operator==(const Predicate &pc) { return false; } //TODO
} // namespace CCPP