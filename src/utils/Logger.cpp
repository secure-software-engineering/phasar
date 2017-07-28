/*
 * Logger.cpp
 *
 *  Created on: 27.07.2017
 *      Author: philipp
 */

#include "Logger.hh"

ostream &operator<<(ostream &os, enum severity_level l) {
  static const array<string, 5> strings = {
      {"DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL"}};
  return os << strings.at(l);
}

bool LogFilter(const attribute_value_set &set) {
  return set["Severity"].extract<severity_level>() >= 0;
}

void LogFormatter(const record_view &view, formatting_ostream &os) {
  os << view.attribute_values()["LineCounter"].extract<int>() << " ( "
     << view.attribute_values()["Timestamp"].extract<boost::posix_time::ptime>()
     << ")"
     << " - level "
     << view.attribute_values()["Severity"].extract<severity_level>() << ": "
     << view.attribute_values()["Message"].extract<std::string>();
}

void LoggerExceptionHandler::operator()(const std::exception &ex) const {
  std::cerr << "std::exception: " << ex.what() << '\n';
}

void initializeLogger() {
  // Using this call, logging is completely disabled
  // core::get()->set_logging_enabled(false);
  typedef sinks::asynchronous_sink<sinks::text_ostream_backend> text_sink;
  boost::shared_ptr<text_sink> sink = boost::make_shared<text_sink>();
  // we could also use a output file stream of course
  boost::shared_ptr<std::ostream> stream{&std::clog, boost::empty_deleter{}};
  // auto stream = boost::make_shared<std::ofstream>("mylogfile.log");
  sink->locked_backend()->add_stream(stream);
  sink->set_filter(&LogFilter);
  sink->set_formatter(&LogFormatter);
  core::get()->add_sink(sink);
  core::get()->add_global_attribute("LineCounter", attributes::counter<int>{});
  core::get()->add_global_attribute("Timestamp", attributes::local_clock{});
  core::get()->set_exception_handler(
      make_exception_handler<std::exception>(LoggerExceptionHandler()));
}
