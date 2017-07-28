/*
 * Logger.hh
 *
 *  Created on: 27.07.2017
 *      Author: philipp
 */

#ifndef UTILS_LOGGER_HH_
#define UTILS_LOGGER_HH_

#include <array>
#include <boost/log/attributes.hpp>
#include <boost/log/common.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks.hpp>
#include <boost/log/sources/global_logger_storage.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/utility/exception_handler.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/utility/empty_deleter.hpp>
#include <exception>
#include <fstream>
#include <iostream>
#include <string>

using namespace std;
using namespace boost::log;

// Additionally consult:
//  - https://theboostcpplibraries.com/boost.log
//  - http://www.boost.org/doc/libs/1_64_0/libs/log/doc/html/log/tutorial.html

enum severity_level { DEBUG = 0, INFO, WARNING, ERROR, CRITICAL };

ostream &operator<<(ostream &os, enum severity_level l);

// Register the logger and use it a singleton then, get the logger with:
// sources::severity_logger<severity_level>& lg = lg::get();
BOOST_LOG_INLINE_GLOBAL_LOGGER_DEFAULT(lg, sources::severity_logger<severity_level>)
// The logger can also be used as a global variable, which is not recommended.
// In such a case a global variable would be created like in the following
// sources::severity_logger<int> lg;

// A few attributes that we want to use in our logger
BOOST_LOG_ATTRIBUTE_KEYWORD(severity, "Severity", severity_level)
BOOST_LOG_ATTRIBUTE_KEYWORD(counter, "LineCounter", int)
BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "Timestamp", boost::posix_time::ptime)

/**
 * A filter function.
 */
bool logFilter(const attribute_value_set &set);

/**
 * A formatter function.
 */
void logFormatter(const record_view &view, formatting_ostream &os);

/**
 * An exception handler for the logger.
 */
struct LoggerExceptionHandler {
  void operator()(const std::exception &ex) const;
};

/**
 * Initializes the logger.
 */
void initializeLogger();

#endif /* UTILS_LOGGER_HH_ */