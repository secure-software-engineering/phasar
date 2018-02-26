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

#ifndef UTILS_LOGGER_H_
#define UTILS_LOGGER_H_

#include <phasar/Config/Configuration.h>
#include <algorithm>
#include <array>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
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
#include <boost/core/null_deleter.hpp>
#include <ctime>
#include <exception>
#include <fstream>
#include <iostream>
#include <string>

using namespace std;
namespace bl = boost::log;
namespace bfs = boost::filesystem;

// Additionally consult:
//  - https://theboostcpplibraries.com/boost.log
//  - http://www.boost.org/doc/libs/1_64_0/libs/log/doc/html/log/tutorial.html

enum severity_level { DEBUG = 0, INFO, WARNING, ERROR, CRITICAL };

extern const map<string, severity_level> StringToSeverityLevel;

extern const map<severity_level, string> SeverityLevelToString;

ostream &operator<<(ostream &os, enum severity_level l);

// Register the logger and use it a singleton then, get the logger with:
// bl::sources::severity_logger<severity_level>& lg = lg::get();
BOOST_LOG_INLINE_GLOBAL_LOGGER_DEFAULT(
    lg, bl::sources::severity_logger<severity_level>)
// The logger can also be used as a global variable, which is not recommended.
// In such a case a global variable would be created like in the following
// bl::sources::severity_logger<int> lg;

// A few attributes that we want to use in our logger
BOOST_LOG_ATTRIBUTE_KEYWORD(severity, "Severity", severity_level)
BOOST_LOG_ATTRIBUTE_KEYWORD(counter, "LineCounter", int)
BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "Timestamp", boost::posix_time::ptime)

/**
 * A filter function.
 */
bool logFilter(const bl::attribute_value_set &set);

/**
 * A formatter function.
 */
void logFormatter(const bl::record_view &view, bl::formatting_ostream &os);

/**
 * An exception handler for the logger.
 */
struct LoggerExceptionHandler {
  void operator()(const std::exception &ex) const;
};

/**
 * Initializes the logger.
 */
void initializeLogger(bool use_logger);

#endif /* UTILS_LOGGER_HH_ */