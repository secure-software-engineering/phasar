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
#include <type_traits>

#include "boost/log/sinks.hpp"
#include "boost/log/sources/global_logger_storage.hpp"
#include "boost/log/sources/severity_logger.hpp"
#include "boost/log/support/date_time.hpp"
// Not useful here but enable all logging macros in files that include Logger.h
#include "boost/log/sources/record_ostream.hpp"

#include "llvm/IR/Value.h"
#include "llvm/Support/Compiler.h" // LLVM_UNLIKELY
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_os_ostream.h"

namespace psr {

// Additionally consult:
//  - https://theboostcpplibraries.com/boost.log
//  - http://www.boost.org/doc/libs/1_64_0/libs/log/doc/html/log/tutorial.html

enum SeverityLevel {
#define SEVERITY_LEVEL(NAME, TYPE) TYPE,
#include "phasar/Utils/SeverityLevel.def"
  INVALID
};

std::string toString(const SeverityLevel &Level);

SeverityLevel toSeverityLevel(const std::string &S);

std::ostream &operator<<(std::ostream &OS, const SeverityLevel &Level);

extern SeverityLevel LogFilterLevel;

#ifdef DYNAMIC_LOG
BOOST_LOG_INLINE_GLOBAL_LOGGER_DEFAULT(
    lg, boost::log::sources::severity_logger<SeverityLevel>)
// For performance reason, we want to disable any formatting computation
// that would go straight into logs if logs are deactivated
// This macro does just that
#define LOG_IF_ENABLE_BOOL(condition, computation)                             \
  if (LLVM_UNLIKELY(condition)) {                                              \
    computation;                                                               \
  }

#define LOG_IF_ENABLE(computation)                                             \
  LOG_IF_ENABLE_BOOL(boost::log::core::get()->get_logging_enabled(),           \
                     computation)

#define IS_LOG_ENABLED bool(boost::log::core::get()->get_logging_enabled())
// Register the logger and use it a singleton then, get the logger with:
// boost::log::sources::severity_logger<SeverityLevel>& lg = lg::get();

// The logger can also be used as a global variable, which is not recommended.
// In such a case a global variable would be created like in the following
// boost::log::sources::severity_logger<int> lg;

// A few attributes that we want to use in our logger
BOOST_LOG_ATTRIBUTE_KEYWORD(severity, "Severity", SeverityLevel)
BOOST_LOG_ATTRIBUTE_KEYWORD(counter, "LineCounter", int)
BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "Timestamp", boost::posix_time::ptime)

#else
#define LOG_IF_ENABLE_BOOL(condition, computation) ((void)0)
#define LOG_IF_ENABLE(computation) ((void)0)
#define IS_LOG_ENABLED false
// Have a mechanism to prevent logger usage if the code is not compiled using
// the DYNAMIC_LOG option:
template <typename T> struct __lg__ {
  // Make the static assert dependent on a template-parameter to prevent the
  // compiler raising an error on declaration rather than on
  // template-instantiation.
  static_assert(!std::is_same_v<void, T>,
                "The dynamic log is disabled. Please move this call "
                "to lg::get() into LOG_IF_ENABLE, or use the "
                "cmake option '-DPHASAR_ENABLE_DYNAMIC_LOG=ON'.");
  static inline boost::log::sources::severity_logger<SeverityLevel> &get() {
    llvm::report_fatal_error(
        "The dynamic log is disabled. Please move this call "
        "to lg::get() into LOG_IF_ENABLE, or use the "
        "cmake option '-DPHASAR_ENABLE_DYNAMIC_LOG=ON'.");
  }
};
#define lg __lg__<void>
#endif

/**
 * A filter function.
 */
bool logFilter(const boost::log::attribute_value_set &AVSet);

/**
 * Set the filter level.
 */
void setLoggerFilterLevel(SeverityLevel Level);

/**
 * A formatter function.
 */
void logFormatter(const boost::log::record_view &View,
                  boost::log::formatting_ostream &OS);

/**
 * An exception handler for the logger.
 */
struct LoggerExceptionHandler {
  void operator()(const std::exception &Ex) const;
};

/**
 * Initializes the logger.
 */
void initializeLogger(bool UseLogger, const std::string &LogFile = "");

} // namespace psr

#endif
