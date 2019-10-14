#pragma once
#include <FormatConverter/DefinedObject.h>
#include <FormatConverter/Predicate.h>

namespace CCPP {
	class PredicateConverter {
	private:
        Predicate predicate;
		CrySLParser::EnsPredContext *ensCtx; 
       // CrySLParser::PredContext *predCtx;
        CrySLParser::ReqPredContext *reqCtx; 

	public:
        //PredicateConverter(CrySLParser::PredContext *predicateCtxObj);
        PredicateConverter(CrySLParser::EnsPredContext *ensCtxObj);
		PredicateConverter(CrySLParser::ReqPredContext *reqCtxObj);
        std::vector<Predicate> formatConverter();
        };
} // namespace CCPP