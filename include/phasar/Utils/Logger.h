/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * Logger.h
 *
 *  Created on: 27.07.2017
 *      Author: philipp
 */

#ifndef PHASAR_UTILS_LOGGER_H_
#define PHASAR_UTILS_LOGGER_H_

#include <iosfwd>
#include <string>

#include <boost/log/sinks.hpp>
#include <boost/log/sources/global_logger_storage.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/support/date_time.hpp>
// Not useful here but enable all logging macros in files that include Logger.h
#include <boost/log/sources/record_ostream.hpp>

namespace psr {

// Additionally consult:
//  - https://theboostcpplibraries.com/boost.log
//  - http://www.boost.org/doc/libs/1_64_0/libs/log/doc/html/log/tutorial.html

enum severity_level { DEBUG = 0, INFO, WARNING, ERROR, CRITICAL };

extern const std::map<std::string, severity_level> StringToSeverityLevel;

extern const std::map<severity_level, std::string> SeverityLevelToString;

extern severity_level logFilterLevel;

std::ostream &operator<<(std::ostream &os, enum severity_level l);

// For performance reason, we want to disable any formatting computation
// that would go straight into logs if logs are deactivated
// This macro does just that

#define LOG_IF_ENABLE(computation)                                             \
  if (boost::log::core::get()->get_logging_enabled()) {                        \
    computation;                                                               \
  }

// Register the logger and use it a singleton then, get the logger with:
// boost::log::sources::severity_logger<severity_level>& lg = lg::get();
BOOST_LOG_INLINE_GLOBAL_LOGGER_DEFAULT(
    lg, boost::log::sources::severity_logger<severity_level>)
// The logger can also be used as a global variable, which is not recommended.
// In such a case a global variable would be created like in the following
// boost::log::sources::severity_logger<int> lg;

// A few attributes that we want to use in our logger
BOOST_LOG_ATTRIBUTE_KEYWORD(severity, "Severity", severity_level)
BOOST_LOG_ATTRIBUTE_KEYWORD(counter, "LineCounter", int)
BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "Timestamp", boost::posix_time::ptime)

/**
 * A filter function.
 */
bool logFilter(const boost::log::attribute_value_set &set);

/**
 * Set the filter level.
 */
void setLoggerFilterLevel(severity_level level);

/**
 * A formatter function.
 */
void logFormatter(const boost::log::record_view &view,
                  boost::log::formatting_ostream &os);

/**
 * An exception handler for the logger.
 */
struct LoggerExceptionHandler {
  void operator()(const std::exception &ex) const;
};

/**
 * Initializes the logger.
 */
void initializeLogger(bool use_logger, std::string log_file = "");

} // namespace psr

#endif
