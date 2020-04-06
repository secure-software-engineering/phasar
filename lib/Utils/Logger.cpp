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
#include <ctime>
#include <exception>

#include "boost/algorithm/string.hpp"
#include "boost/core/null_deleter.hpp"
#include "boost/filesystem.hpp"
#include "boost/shared_ptr.hpp"

#include "boost/log/attributes.hpp"
#include "boost/log/utility/exception_handler.hpp"

#include "phasar/Utils/Logger.h"
using namespace std;
using namespace psr;

namespace psr {

const map<string, severity_level> StringToSeverityLevel = {
    {"DEBUG", DEBUG},
    {"INFO", INFO},
    {"WARNING", WARNING},
    {"ERROR", ERROR},
    {"CRITICAL", CRITICAL}};

const map<severity_level, string> SeverityLevelToString = {
    {DEBUG, "DEBUG"},
    {INFO, "INFO"},
    {WARNING, "WARNING"},
    {ERROR, "ERROR"},
    {CRITICAL, "CRITICAL"}};

severity_level LogFilterLevel = DEBUG;

void setLoggerFilterLevel(severity_level Level) { LogFilterLevel = Level; }

ostream &operator<<(ostream &OS, enum severity_level L) {
  return OS << SeverityLevelToString.at(L);
}

bool LogFilter(const boost::log::attribute_value_set &Set) {
  return Set["Severity"].extract<severity_level>() >= LogFilterLevel;
}

void LogFormatter(const boost::log::record_view &View,
                  boost::log::formatting_ostream &OS) {
  OS << View.attribute_values()["LineCounter"].extract<int>() << " "
     << View.attribute_values()["Timestamp"].extract<boost::posix_time::ptime>()
     << " - [" << View.attribute_values()["Severity"].extract<severity_level>()
     << "] " << View.attribute_values()["Message"].extract<std::string>();
}

void LoggerExceptionHandler::operator()(const std::exception &Ex) const {
  std::cerr << "std::exception: " << Ex.what() << '\n';
}

void initializeLogger(bool UseLogger, string LogFile) {
  // Using this call, logging can be enabled or disabled
  boost::log::core::get()->set_logging_enabled(UseLogger);
  // if (log_file == "") {
  typedef boost::log::sinks::synchronous_sink<
      boost::log::sinks::text_ostream_backend>
      text_sink;
  boost::shared_ptr<text_sink> Sink = boost::make_shared<text_sink>();
  // the easiest way is to write the logs to std::clog
  boost::shared_ptr<std::ostream> Stream(&std::clog, boost::null_deleter{});
  // } else {
  // // get the time and make it into a string for log file nameing
  // time_t current_time = std::time(nullptr);
  // string time(asctime(localtime(&current_time)));
  // transform(time.begin(), time.end(), time.begin(),
  //           [](char c) { return c == ' ' ? '_' : c; });
  // boost::trim_right(time);
  // // check if the log directory exists, otherwise create it
  // if (!(bfs::exists(LogFileDirectory))) {
  //   bfs::create_directory(LogFileDirectory);
  // }
  // // we could also use a output file stream of course
  // auto stream = boost::make_shared<std::ofstream>(LogFileDirectory + time +
  // log_file);
  // }
  Sink->locked_backend()->add_stream(Stream);
  Sink->set_filter(&LogFilter);
  Sink->set_formatter(&LogFormatter);
  boost::log::core::get()->add_sink(Sink);
  boost::log::core::get()->add_global_attribute(
      "LineCounter", boost::log::attributes::counter<int>{});
  boost::log::core::get()->add_global_attribute(
      "Timestamp", boost::log::attributes::local_clock{});
  boost::log::core::get()->set_exception_handler(
      boost::log::make_exception_handler<std::exception>(
          LoggerExceptionHandler()));
}
} // namespace psr
