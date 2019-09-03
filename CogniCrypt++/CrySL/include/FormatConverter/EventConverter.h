#pragma once
#include "Types/Type.h"
#include "Object.h"
#include "Event.h"

namespace CCPP {

	class EventConverter {
        std::vector<Object> objects;
        Event event;

		void formatConverter(CrySLParser::EventsContext* eventCtx);
     };

}