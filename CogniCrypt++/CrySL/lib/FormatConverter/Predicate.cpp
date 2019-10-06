#include "Predicate.h"
#include <isotream>
#include <FormatConverter/Predicate.h>
#include <FormatConverter/PredicateConverter.h>

namespace CCPP {
PredicateConverter::PredicateConverter(CrySLParser::PredContext *predicateCtxObj) {
	this->predCtx = predicateCtxObj;
}
}