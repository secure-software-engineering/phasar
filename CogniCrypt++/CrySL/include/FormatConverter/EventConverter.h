#pragma once
#include "Event.h"
#include "Object.h"
#include "Types/Type.h"


namespace CCPP {

class EventConverter {
  std::vector<Object> objects;
  Event event;

  std::vector<Event> formatConverter(CrySLParser::EventsContext *eventCtx);
};

} // namespace CCPP