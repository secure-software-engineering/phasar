
#include "CrySLTypechecker.h"
#include <Event.h>
#include <EventConverter.h>
#include <iostream>

namespace CCPP {

	void EventConverter::formatConverter(CrySLParser::EventsContext *eventCtx) {
	  EventConverter eventConverter;
	  Event eventObj;
	  std::vector<Object> objects;
	  std::vector<Event> events;

	  for (auto event : eventCtx->eventsOccurence()) {
		eventObj.eventName = event->eventName->getText();

		for (auto param : event->parametersList()->param()) {
                  objects.emplace_back(param);	//creates object of type Object using parameterize constructor with param as a parameter and inserts it in vector
		}

		checkFactoryConsumerFunc(event, eventObj);

		events.push_back(eventObj);
	  }
	}

	void checkFactoryConsumerFunc(auto event, Event eventObj) {
	  if (event->returnValue && event->returnValue->getText() == "this") {
			eventObj.isFactoryFunction = true;
			eventObj.factoryParamIdx = {-1};
	  } 
	  else {
            eventObj.isConsumingFunction = true;

            int index = 0;
            std::set<double> consumerParamIdx;
            for (auto param : event->parametersList()->param()) {
				
             if (param->getText() == "this") {
					consumerParamIdx.insert(index);
              }
              index++;
			}
            eventObj.consumerParamIdx = consumerParamIdx;
		}
	}

} // namespace CCPP