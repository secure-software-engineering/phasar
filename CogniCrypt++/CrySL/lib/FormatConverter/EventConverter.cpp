
#include "CrySLTypechecker.h"
#include <iostream>
#include<EventConverter.h>
#include<Event.h>

namespace CCPP {

	void EventConverter::formatConverter(CrySLParser::EventsContext* eventCtx) {
		  EventConverter eventConverter;
		  Event eventObj;
          std::vector<Object> objects;
          std::vector<Event> events;

		  for (auto event : evt->eventsOccurence()) {
                 eventObj.eventName = event->eventName->getText();
                 events.insert(eventObj);

                 for (auto param : event->parametersList()->param()) {
					
                 }
		}
	}
}