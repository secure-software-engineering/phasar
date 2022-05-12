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

#include <chrono>
#include <iosfwd>
#include <optional>
#include <string>
#include <type_traits>
#include <variant>

#include "llvm/ADT/StringMap.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/Compiler.h" // LLVM_UNLIKELY
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_os_ostream.h"
#include "llvm/Support/raw_ostream.h"

namespace psr {

// Additionally consult:
//  - https://theboostcpplibraries.com/boost.log
//  - http://www.boost.org/doc/libs/1_64_0/libs/log/doc/html/log/tutorial.html

enum SeverityLevel {
#define SEVERITY_LEVEL(NAME, TYPE) TYPE,
#include "phasar/Utils/SeverityLevel.def"
  INVALID
};

class Logger final {
public:
  /**
   * Set the filter level.
   */
  static void setLoggerFilterLevel(SeverityLevel Level);

  static SeverityLevel getLoggerFilterLevel();

  static bool isLoggingEnabled();

  static void enable();

  static void disable();

  static llvm::raw_ostream &
  getLogStream(const std::optional<SeverityLevel> &Level,
               const std::optional<llvm::StringRef> &Category);

  static bool logCategory(const llvm::StringRef &Category,
                          const std::optional<SeverityLevel> &Level);

  static void addLinePrefix(llvm::raw_ostream &,
                            const std::optional<SeverityLevel> &Level,
                            const std::optional<std::string> &Category);

  static void initializeStdoutLogger(
      const std::optional<SeverityLevel> &Level = std::nullopt,
      const std::optional<std::string> &Category = std::nullopt);

  static void initializeStderrLogger(
      const std::optional<SeverityLevel> &Level = std::nullopt,
      const std::optional<std::string> &Category = std::nullopt);

  [[nodiscard]] static bool initializeFileLogger(
      const llvm::StringRef &Filename,
      const std::optional<SeverityLevel> &Level = std::nullopt,
      const std::optional<std::string> &Category = std::nullopt,
      bool Append = false);

private:
  enum class StdStream : uint8_t { STDOUT = 0, STDERR };
  static inline bool LoggingEnabled = false;
  static inline llvm::StringMap<std::map<std::optional<SeverityLevel>,
                                         std::variant<StdStream, std::string>>>
      CategoriesToStreamVariant;
  static inline std::map<std::optional<SeverityLevel>,
                         std::variant<StdStream, std::string>>
      LevelsToStreamVariant;
  static inline SeverityLevel LogFilterLevel = DEBUG;
  static inline llvm::StringMap<std::unique_ptr<llvm::raw_fd_ostream>>
      LogfileStreams;
  // static inline auto StartTime = std::chrono::steady_clock::now();
  [[nodiscard]] static llvm::raw_ostream &
  getLogStream(const std::optional<SeverityLevel> &Level,
               const std::map<std::optional<SeverityLevel>,
                              std::variant<StdStream, std::string>>
                   &PassedLevelsToStreamVariant);
  [[nodiscard]] static llvm::raw_ostream &getLogStreamFromStreamVariant(
      const std::variant<StdStream, std::string> &StreamVariant);
};

std::string toString(const SeverityLevel &Level);

#ifdef DYNAMIC_LOG

#define PHASAR_LOG(message) PHASAR_LOG_LEVEL(DEBUG, message)

#define PHASAR_LOG_LEVEL(level, message)                                       \
  IF_LOG_ENABLED_BOOL(                                                         \
      Logger::isLoggingEnabled() && level >= Logger::getLoggerFilterLevel(),   \
      do {                                                                     \
        auto &S = Logger::getLogStream(level, std::nullopt);                   \
        Logger::addLinePrefix(S, level, std::nullopt);                         \
        S << message << '\n';                                                  \
      } while (false);)

#define PHASAR_LOG_LEVEL_CAT(level, cat, message)                              \
  IF_LOG_ENABLED_BOOL(                                                         \
      Logger::isLoggingEnabled() && level >= Logger::getLoggerFilterLevel() && \
          Logger::logCategory(cat, level),                                     \
      do {                                                                     \
        auto &S = Logger::getLogStream(level, cat);                            \
        Logger::addLinePrefix(S, level, cat);                                  \
        S << message << '\n';                                                  \
      } while (false);)

#define PHASAR_LOG_CAT(cat, message)                                           \
  IF_LOG_ENABLED_BOOL(                                                         \
      Logger::isLoggingEnabled() && Logger::logCategory(cat, std::nullopt),    \
      do {                                                                     \
        auto &S = Logger::getLogStream(std::nullopt, cat);                     \
        Logger::addLinePrefix(S, std::nullopt, cat);                           \
        S << message << '\n';                                                  \
      } while (false);)

// For performance reason, we want to disable any
// formatting computation that would go straight into
// logs if logs are deactivated This macro does just
// that
#define IF_LOG_ENABLED_BOOL(condition, computation)                            \
  if (LLVM_UNLIKELY(condition)) {                                              \
    computation;                                                               \
  }

// #define LOG_IF_ENABLE(computation)                                             \
//   IF_LOG_ENABLED_BOOL(Logger::isLoggingEnabled(), computation)

#define IF_LOG_ENABLED(computation)                                            \
  IF_LOG_ENABLED_BOOL(Logger::isLoggingEnabled(), computation)

#define IS_LOG_ENABLED Logger::isLoggingEnabled()
// Register the logger and use it a singleton then, get the logger with:
// boost::log::sources::severity_logger<SeverityLevel>& lg = lg::get();

// The logger can also be used as a global variable, which is not recommended.
// In such a case a global variable would be created like in the following
// boost::log::sources::severity_logger<int> lg;

#else
#define IF_LOG_ENABLED_BOOL(condition, computation) ((void)0)
#define PHASAR_LOG(computation) ((void)0)
#define PHASAR_LOG_CAT(cat, message) ((void)0)
#define PHASAR_LOG_LEVEL_CAT(level, cat, message) ((void)0)
#define PHASAR_LOG_LEVEL(level, message) ((void)0)
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
 * Initializes the logger.
 */
[[deprecated("Please use the new initialize*Logger() family instead")]] void
initializeLogger(bool UseLogger, const std::string &LogFile = "");

} // namespace psr

#endif
