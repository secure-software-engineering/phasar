
#include "CrySLTypechecker.h"
#include "Types/Type.h"
#include <iostream>
#include "PositionHelper.h"

bool checkEventObjects(CrySLParser::ParametersListContext* paramList, std::unordered_map<std::string, std::shared_ptr<CCPP::Types::Type>>& definedObjects) {
	bool result = true;
	for (auto prm : paramList->param())
	{
		if (prm->memberAccess()) {
			auto parameters = prm->memberAccess()->Ident();

			for (auto pObject : parameters)
			{
				if (!definedObjects.count(pObject->getText())) {
					std::cerr << Position(pObject) << ": object does not exist in the objects section"<<std::endl;
					return false;
				}
			}
		}
		else if(!prm->thisPtr || !prm->wildCard) {
			return true;
		}
	}
	return result;
}

bool CCPP::CrySLTypechecker::CrySLSpec::typecheck(CrySLParser::EventsContext* evt) {

	bool result = true;

	for (auto event : evt->eventsOccurence()) {
		auto eventName = event->eventName->getText();

		if (!event->returnValue) {
			if (!DefinedObjects.count(event->returnValue->getText())) {
				return false;
			}
		}

		if (DefinedEvents.count(eventName)) {
			result = checkEventObjects(event->parametersList(), DefinedObjects);
		}
		else {
			DefinedEvents.insert(eventName);
			result = checkEventObjects(event->parametersList(), DefinedObjects);
		}
		return result;
	}
}

