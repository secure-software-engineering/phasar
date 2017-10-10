/*
 * Logger.cpp
 *
 *  Created on: 27.07.2017
 *      Author: philipp
 */

#include "Logger.hh"

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

ostream &operator<<(ostream &os, enum severity_level l) {
  return os << SeverityLevelToString.at(l);
}

bool LogFilter(const bl::attribute_value_set &set) {
  return set["Severity"].extract<severity_level>() >= 0;
}

void LogFormatter(const bl::record_view &view, bl::formatting_ostream &os) {
  os << view.attribute_values()["LineCounter"].extract<int>() << " "
     << view.attribute_values()["Timestamp"].extract<boost::posix_time::ptime>()
     << " - level ["
     << view.attribute_values()["Severity"].extract<severity_level>() << "] "
     << view.attribute_values()["Message"].extract<std::string>();
}

void LoggerExceptionHandler::operator()(const std::exception &ex) const {
  std::cerr << "std::exception: " << ex.what() << '\n';
}

void initializeLogger(bool use_logger) {
  // Using this call, logging can be enabled or disabled
  bl::core::get()->set_logging_enabled(use_logger);
  typedef bl::sinks::asynchronous_sink<bl::sinks::text_ostream_backend>
      text_sink;
  boost::shared_ptr<text_sink> sink = boost::make_shared<text_sink>();
  // the easiest way is to write the logs to std::clog
  boost::shared_ptr<std::ostream> stream(&std::clog, boost::empty_deleter{});
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
  // ".log");
  sink->locked_backend()->add_stream(stream);
  sink->set_filter(&LogFilter);
  sink->set_formatter(&LogFormatter);
  bl::core::get()->add_sink(sink);
  bl::core::get()->add_global_attribute("LineCounter",
                                        bl::attributes::counter<int>{});
  bl::core::get()->add_global_attribute("Timestamp",
                                        bl::attributes::local_clock{});
  bl::core::get()->set_exception_handler(
      bl::make_exception_handler<std::exception>(LoggerExceptionHandler()));
}
