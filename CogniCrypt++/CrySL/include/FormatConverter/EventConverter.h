#pragma once
#include <FormatConverter/Event.h>
//#include <FormatConverter/ObjectWithLLVM.h>
#include <FormatConverter/ObjectWithOutLLVM.h>

namespace CCPP {

class EventConverter {
  std::vector<ObjectWithOutLLVM> objects;
  Event event;

  std::vector<Event> formatConverter(CrySLParser::EventsContext *eventCtx);
};

} // namespace CCPP