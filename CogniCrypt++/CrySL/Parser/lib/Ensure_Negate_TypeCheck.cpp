#include "CrySLTypechecker.h"
#include "Types/Type.h"
#include <iostream>
#include "PositionHelper.h"

bool checkPredicate(CrySLParser::EnsPredContext *ensu, std::unordered_map<std::string, std::shared_ptr<CCPP::Types::Type>> & DefinedObjects)
{
	auto predFull = ensu->pred();
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
bool CCPP::CrySLTypechecker::CrySLSpec::typecheck(CrySLParser::EnsuresContext* ens) {
	bool result = true;

	for (auto ensu : ens->ensPred())
	{
		EnsuredPreds.push_back(ensu);
		result &= checkPredicate(ensu, DefinedObjects);

		
	}
	return result;
}

bool CCPP::CrySLTypechecker::CrySLSpec::typecheck(CrySLParser::NegatesContext* neg) {
	bool result = true;

	for (auto ensu : neg->negatesOccurence())
	{
		NegatedPreds.push_back(ensu->ensPred());
		result &= checkPredicate(ensu->ensPred(), DefinedObjects);


	}
	return result;
}