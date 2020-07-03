/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * Logger.cpp
 *
 *  Created on: 27.07.2017
 *      Author: philipp
 */

#include <algorithm>
#include <array>
#include <exception>

#include "boost/algorithm/string.hpp"
#include "boost/core/null_deleter.hpp"
#include "boost/filesystem.hpp"
#include "boost/log/attributes.hpp"
#include "boost/log/utility/exception_handler.hpp"
#include "boost/shared_ptr.hpp"

#include "llvm/ADT/StringSwitch.h"

#include "phasar/Utils/Logger.h"

using namespace std;
using namespace psr;

namespace psr {

SeverityLevel LogFilterLevel = DEBUG;

std::string toString(const SeverityLevel &Level) {
  switch (Level) {
  default:
#define SEVERITY_LEVEL(NAME, TYPE)                                             \
  case SeverityLevel::TYPE:                                                    \
    return NAME;                                                               \
    break;
#include "phasar/Utils/SeverityLevel.def"
  }
}

SeverityLevel toSeverityLevel(const std::string &S) {
  SeverityLevel Type = llvm::StringSwitch<SeverityLevel>(S)
#define SEVERITY_LEVEL(NAME, TYPE) .Case(NAME, SeverityLevel::TYPE)
#include "phasar/Utils/SeverityLevel.def"
                           .Default(SeverityLevel::INVALID);
  return Type;
}

std::ostream &operator<<(std::ostream &OS, const SeverityLevel &Level) {
  return OS << toString(Level);
}

void setLoggerFilterLevel(SeverityLevel Level) { LogFilterLevel = Level; }

bool logFilter(const boost::log::attribute_value_set &Set) {
#ifdef DYNAMIC_LOG
  return Set["Severity"].extract<SeverityLevel>() >= LogFilterLevel;
#else
  return false;
#endif
}

void logFormatter(const boost::log::record_view &View,
                  boost::log::formatting_ostream &OS) {
#ifdef DYNAMIC_LOG
  OS << View.attribute_values()["LineCounter"].extract<int>()
     << " "
     //  <<
     //  View.attribute_values()["Timestamp"].extract<boost::posix_time::ptime>()
     << " - [" << View.attribute_values()["Severity"].extract<SeverityLevel>()
     << "] " << View.attribute_values()["Message"].extract<std::string>();
#endif
}

void initializeLogger(bool UseLogger, const string &LogFile) {
#ifdef DYNAMIC_LOG
  // Using this call, logging can be enabled or disabled
  boost::log::core::get()->set_logging_enabled(UseLogger);
  using text_sink = boost::log::sinks::synchronous_sink<
      boost::log::sinks::text_ostream_backend>;
  boost::shared_ptr<text_sink> Sink = boost::make_shared<text_sink>();
  // boost::shared_ptr<std::ostream> Stream = nullptr;
  // if (LogFile.empty()) {
  //  // the easiest way is to write the logs to std::clog
  boost::shared_ptr<std::ostream> Stream(&std::clog, boost::null_deleter{});
  // } else {
  // Stream = boost::make_shared<std::ofstream>(LogFile);
  // }
  Sink->locked_backend()->add_stream(Stream);
  Sink->set_filter(&logFilter);
  Sink->set_formatter(&logFormatter);
  boost::log::core::get()->add_sink(Sink);
  boost::log::core::get()->add_global_attribute(
      "LineCounter", boost::log::attributes::counter<int>{});
  boost::log::core::get()->add_global_attribute(
      "Timestamp", boost::log::attributes::local_clock{});
  boost::log::core::get()->set_exception_handler(
      boost::log::make_exception_handler<std::exception>(
          LoggerExceptionHandler()));
#endif
}

void LoggerExceptionHandler::operator()(const std::exception &Ex) const {
  std::cerr << "std::exception: " << Ex.what() << '\n';
}

} // namespace psr
