#include "CrySLTypechecker.h"
#include "Types/Type.h"
#include <iostream>
#include "PositionHelper.h"

bool checkPredicate(CrySLParser::ReqPredContext* requ, std::unordered_map<std::string, std::shared_ptr<CCPP::Types::Type>>& DefinedObjects)
{
	auto predFull = requ->neg;
	if (predFull->suParList())
	{
		auto parameters = predFull->suParList()->suPar();
		for (auto perP : parameters)
		{
			if (perP->consPred() && perP->consPred()->literalExpr() && perP->consPred()->literalExpr()->memberAccess())
			{
				for (auto idts : perP->consPred()->literalExpr()->memberAccess()->Ident())
				{
					if (!DefinedObjects.count(idts->getText()))
					{
						std::cerr << Position(idts) << ": object is not defined in the OBJECTS section" << std::endl;
						return false;
					}
				}
			}
		}
	}
	return true;
}
bool CCPP::CrySLTypechecker::CrySLSpec::typecheck(CrySLParser::RequiresBlockContext* req)
{
	bool result = true;

	for (auto requ : req->reqPred())
	{
		RequiredPreds.push_back(requ);
		result &= checkPredicate(requ, DefinedObjects);


	}
	return result;
}