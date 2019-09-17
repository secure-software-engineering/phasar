
#include <FormatConverter/Event.h>
#include <FormatConverter/EventConverter.h>
#include <iostream>

namespace CCPP {
void checkFactoryConsumerFunc(CrySLParser::EventsOccurenceContext*  event, Event &eventObj);
// this function extracts events from AST node of eventscontext and convert them
// into Event entity class
std::vector<Event>
EventConverter::formatConverter(CrySLParser::EventsContext *eventCtx) {
  Event eventObj;
  std::vector<ObjectWithOutLLVM> params;
  std::vector<Event> events;

  for (auto event : eventCtx->eventsOccurence()) {
    eventObj.setEventName(event->eventName->getText());

    for (auto param : event->parametersList()->param()) {
      params.emplace_back(param); // creates object of type ObjectWithOutLLVM using
                                   // parameterize constructor with param as a
                                   // parameter and inserts it in vector
    }
    eventObj.setParams(params);
    checkFactoryConsumerFunc(
        event, eventObj); // checks if the event is factory or consumer funciton
                          // and finds it's event
    events.push_back(eventObj);
  }
  return events;
}

void checkFactoryConsumerFunc(CrySLParser::EventsOccurenceContext* event, Event &eventObj) {
  if (event->returnValue && event->returnValue->getText() == "this") {
    eventObj.setIsFactoryFunction(true);
    eventObj.setFactoryParamIdx({-1});
  } else {
    eventObj.setIsConsumingFunction(true);

    int index = 0;
    std::set<int>
        consumerParamIdx; // we need it because this object can occurr multiple
                          // times as a parameter of event
    for (auto param : event->parametersList()->param()) {

      if (param->getText() == "this") {
        consumerParamIdx.insert(index);
      }
      index++;
    }
    eventObj.setConsumerParamIdx(consumerParamIdx);
  }
}

} // namespace CCPP