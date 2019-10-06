#pragma once
#include <FormatConverter/ObjectWithOutLLVM.h>
#include <FormatConverter/Predicate.h>

namespace CCPP {
	class PredicateConverter {
	private:
        Predicate predicate;
		//CrySLParser::EnsPredContext *ensCtx; //not sure, will be used here or not..
        CrySLParser::PredContext *predCtx;
        //CrySLParser::ReqPredContext *reqCtx; //not sure will be used here or not

	public:
        PredicateConverter(CrySLParser::PredContext *predicateCtxObj);
       // PredicateConverter(CrySLParser::EnsPredContext *ensCtxObj);
       // PredicateConverter(CrySLParser::ReqPredContext *reqCtxObj);
        std::vector<Predicate> formatConverter();
        };
} // namespace CCPP