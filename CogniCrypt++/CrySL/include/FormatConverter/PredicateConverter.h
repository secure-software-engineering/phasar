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
		const std::string specName;

	public:
        //PredicateConverter(CrySLParser::PredContext *predicateCtxObj);
        PredicateConverter(CrySLParser::EnsPredContext *ensCtxObj, CrySLParser::ObjectsContext *objCtx, const std::string &specName);
        PredicateConverter(CrySLParser::ReqPredContext *reqCtxObj,
                             CrySLParser::ObjectsContext *objCtx,
                             const std::string &specName);
        std::vector<Predicate> formatConverter();
        };
} // namespace CCPP