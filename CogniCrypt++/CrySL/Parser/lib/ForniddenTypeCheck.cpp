#include "CrySLTypechecker.h"
#include "Types/Type.h"
#include <iostream>
#include "PositionHelper.h"

bool CCPP::CrySLTypechecker::CrySLSpec::typecheck(CrySLParser::ForbiddenContext* forb) {
	bool result = true;

	for (auto forbidden : forb->forbiddenOccurence())
	{
		if (!DefinedEvents.count(forbidden->fqn()->getText()))
		{
			std::cerr << Position(forbidden) << ":Forbidden event does not exist in the event section" << std::endl;
			return false;
		}
	}
	return result;
}