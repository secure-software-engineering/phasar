/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Martin Mory, Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_UTILS_LOGGER_H
#define PHASAR_UTILS_LOGGER_H

#include "phasar/Config/phasar-config.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Compiler.h" // LLVM_UNLIKELY
#include "llvm/Support/raw_ostream.h"

#include <optional>
#include <string>

namespace psr {

enum SeverityLevel {
#define SEVERITY_LEVEL(NAME, TYPE) TYPE,
#include "phasar/Utils/SeverityLevel.def"
  INVALID
};

[[nodiscard]] SeverityLevel parseSeverityLevel(llvm::StringRef Str) noexcept;
[[nodiscard]] llvm::StringRef to_string(SeverityLevel Level) noexcept;

class Logger final {
public:
  /**
   * Set the filter level.
   */
  static void setLoggerFilterLevel(SeverityLevel Level) noexcept;

  static SeverityLevel getLoggerFilterLevel() noexcept {
    return LogFilterLevel;
  }

  static bool isLoggingEnabled() noexcept { return LoggingEnabled; }

  static void enable() noexcept { LoggingEnabled = true; };
  static void disable() noexcept { LoggingEnabled = false; };

  static llvm::raw_ostream &
  getLogStream(std::optional<SeverityLevel> Level,
               const std::optional<llvm::StringRef> &Category);

  static bool logCategory(llvm::StringRef Category,
                          std::optional<SeverityLevel> Level) noexcept;

  static void addLinePrefix(llvm::raw_ostream &,
                            std::optional<SeverityLevel> Level,
                            const std::optional<std::string> &Category);

  static void initializeStdoutLogger(
      std::optional<SeverityLevel> Level = std::nullopt,
      const std::optional<std::string> &Category = std::nullopt);

  static void initializeStderrLogger(
      std::optional<SeverityLevel> Level = std::nullopt,
      const std::optional<std::string> &Category = std::nullopt);

  [[nodiscard]] static bool initializeFileLogger(
      llvm::StringRef Filename,
      std::optional<SeverityLevel> Level = std::nullopt,
      const std::optional<std::string> &Category = std::nullopt,
      bool Append = false);

private:
  static inline bool LoggingEnabled = false;
  static inline SeverityLevel LogFilterLevel = CRITICAL;
};

#ifdef DYNAMIC_LOG

// For performance reason, we want to disable any
// formatting computation that would go straight into
// logs if logs are deactivated. This macro does just
// that
#define IF_LOG_ENABLED_BOOL(condition, computation)                            \
  if (LLVM_UNLIKELY(condition)) {                                              \
    computation;                                                               \
  }

#define IS_LOG_ENABLED ::psr::Logger::isLoggingEnabled()
#define IF_LOG_ENABLED(computation)                                            \
  IF_LOG_ENABLED_BOOL(::psr::Logger::isLoggingEnabled(), computation)

#define IS_LOG_LEVEL_ENABLED(level)                                            \
  (::psr::Logger::isLoggingEnabled() &&                                        \
   (::psr::SeverityLevel::level) >= ::psr::Logger::getLoggerFilterLevel())
#define IF_LOG_LEVEL_ENABLED(level, computation)                               \
  IF_LOG_ENABLED_BOOL(IS_LOG_LEVEL_ENABLED(level), computation)

#define PHASAR_LOG_LEVEL(level, message)                                       \
  IF_LOG_ENABLED_BOOL(                                                         \
      IS_LOG_LEVEL_ENABLED(level), do {                                        \
        auto &Stream = ::psr::Logger::getLogStream(                            \
            ::psr::SeverityLevel::level, std::nullopt);                        \
        ::psr::Logger::addLinePrefix(Stream, ::psr::SeverityLevel::level,      \
                                     std::nullopt);                            \
        /* NOLINTNEXTLINE(bugprone-macro-parentheses) */                       \
        Stream << message << '\n';                                             \
      } while (false);)

#define PHASAR_LOG(message) PHASAR_LOG_LEVEL(DEBUG, message)

#define PHASAR_LOG_LEVEL_CAT(level, cat, message)                              \
  IF_LOG_ENABLED_BOOL(                                                         \
      ::psr::Logger::isLoggingEnabled() &&                                     \
          (::psr::SeverityLevel::level) >=                                     \
              ::psr::Logger::getLoggerFilterLevel() &&                         \
          ::psr::Logger::logCategory(cat, ::psr::SeverityLevel::level),        \
      do {                                                                     \
        auto &Stream =                                                         \
            ::psr::Logger::getLogStream(::psr::SeverityLevel::level, cat);     \
        ::psr::Logger::addLinePrefix(Stream, ::psr::SeverityLevel::level,      \
                                     cat);                                     \
        /* NOLINTNEXTLINE(bugprone-macro-parentheses) */                       \
        Stream << message << '\n';                                             \
      } while (false);)

#define PHASAR_LOG_CAT(cat, message)                                           \
  IF_LOG_ENABLED_BOOL(                                                         \
      ::psr::Logger::isLoggingEnabled() &&                                     \
          ::psr::Logger::logCategory(cat, std::nullopt),                       \
      do {                                                                     \
        auto &Stream = ::psr::Logger::getLogStream(std::nullopt, cat);         \
        ::psr::Logger::addLinePrefix(Stream, std::nullopt, cat);               \
        /* NOLINTNEXTLINE(bugprone-macro-parentheses) */                       \
        Stream << message << '\n';                                             \
      } while (false);)

#else
#define IS_LOG_ENABLED false
#define IF_LOG_ENABLED_BOOL(condition, computation)                            \
  {}
#define IF_LOG_ENABLED(computation)                                            \
  {}
#define IS_LOG_LEVEL_ENABLED(level) false
#define IF_LOG_LEVEL_ENABLED(level, computation)                               \
  {}
#define PHASAR_LOG(computation)                                                \
  {}
#define PHASAR_LOG_CAT(cat, message)                                           \
  {}
#define PHASAR_LOG_LEVEL_CAT(level, cat, message)                              \
  {}
#define PHASAR_LOG_LEVEL(level, message)                                       \
  {}
#endif

/**
 * Initializes the logger.
 */
[[deprecated("Please use the new initialize*Logger() family instead")]] void
initializeLogger(bool UseLogger, const std::string &LogFile = "");

} // namespace psr

#endif
