#pragma once
#include "Types/Type.h"
#include "Object.h"
#include "Event.h"
#include ""

namespace CCPP {

	class EventConverter {
        std::vector<Object> objects;
        Event event;

		void formatConverter(CrySLParser::EventsContext* eventCtx);
     };

}