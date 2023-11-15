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

#include "phasar/Utils/Logger.h"

#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/Support/FileSystem.h"

#include <map>
#include <variant>

auto psr::parseSeverityLevel(llvm::StringRef Str) noexcept -> SeverityLevel {
  return llvm::StringSwitch<SeverityLevel>(Str)
#define SEVERITY_LEVEL(NAME, TYPE) .Case(NAME, SeverityLevel::TYPE)
#include "phasar/Utils/SeverityLevel.def"
      .Default(SeverityLevel::INVALID);
}

llvm::StringRef psr::to_string(SeverityLevel Level) noexcept {
  switch (Level) {
  default:
#define SEVERITY_LEVEL(NAME, TYPE)                                             \
  case SeverityLevel::TYPE:                                                    \
    return NAME;                                                               \
    break;
#include "phasar/Utils/SeverityLevel.def"
  }
}

namespace psr {
namespace logger {

enum class StdStream : uint8_t { STDOUT = 0, STDERR };

static llvm::StringMap<std::map<std::optional<SeverityLevel>,
                                std::variant<StdStream, std::string>>>
    CategoriesToStreamVariant;
static std::map<std::optional<SeverityLevel>,
                std::variant<StdStream, std::string>>
    LevelsToStreamVariant;

static llvm::StringMap<llvm::raw_fd_ostream> LogfileStreams;
// static inline auto StartTime = std::chrono::steady_clock::now();

// ---

[[nodiscard]] static llvm::raw_ostream &getLogStreamFromStreamVariant(
    const std::variant<logger::StdStream, std::string> &StreamVariant) {
  using namespace logger;
  if (std::holds_alternative<StdStream>(StreamVariant)) {
    auto StdStreamKind = std::get<StdStream>(StreamVariant);
    if (StdStreamKind == StdStream::STDOUT) {
      return llvm::outs();
    }
    if (StdStreamKind == StdStream::STDERR) {
      return llvm::errs();
    }
    return llvm::nulls();
  }
  auto It = LogfileStreams.find(std::get<std::string>(StreamVariant));
  assert(It != LogfileStreams.end());
  return It->second;
}

[[nodiscard]] static llvm::raw_ostream &
getLogStream(std::optional<SeverityLevel> Level,
             const std::map<std::optional<SeverityLevel>,
                            std::variant<logger::StdStream, std::string>>
                 &PassedLevelsToStreamVariant) {
  using namespace logger;
  if (Level.has_value()) {
    std::optional<SeverityLevel> ClosestLevel = std::nullopt;
    for (const auto &[LevelThreshold, _] : PassedLevelsToStreamVariant) {
      if (LevelThreshold <= Level.value()) {
        if (!ClosestLevel || ClosestLevel.value() < LevelThreshold) {
          ClosestLevel = LevelThreshold;
        }
      }
    }
    auto StreamVariantIt = PassedLevelsToStreamVariant.find(ClosestLevel);
    if (StreamVariantIt != PassedLevelsToStreamVariant.end()) {
      return getLogStreamFromStreamVariant(StreamVariantIt->second);
    }
    return llvm::nulls();
  }
  auto StreamVariantIt = PassedLevelsToStreamVariant.find(Level);
  if (StreamVariantIt != PassedLevelsToStreamVariant.end()) {
    return getLogStreamFromStreamVariant(StreamVariantIt->second);
  }
  return llvm::nulls();
}
} // namespace logger

void Logger::setLoggerFilterLevel(SeverityLevel Level) noexcept {
  assert(Level >= SeverityLevel::DEBUG && Level < SeverityLevel::INVALID);
  LogFilterLevel = Level;
}

void Logger::initializeStdoutLogger(
    std::optional<SeverityLevel> Level,
    const std::optional<std::string> &Category) {
  LoggingEnabled = true;
  using namespace logger;
  if (Category.has_value()) {
    CategoriesToStreamVariant[*Category][Level] = StdStream::STDOUT;
  } else {
    LevelsToStreamVariant[Level] = StdStream::STDOUT;
  }
  LogFilterLevel = std::min(LogFilterLevel, Level.value_or(CRITICAL));
}

void Logger::initializeStderrLogger(
    std::optional<SeverityLevel> Level,
    const std::optional<std::string> &Category) {
  using namespace logger;
  LoggingEnabled = true;
  if (Category.has_value()) {
    CategoriesToStreamVariant[*Category][Level] = StdStream::STDERR;
  } else {
    LevelsToStreamVariant[Level] = StdStream::STDERR;
  }
  LogFilterLevel = std::min(LogFilterLevel, Level.value_or(CRITICAL));
}

[[nodiscard]] bool Logger::initializeFileLogger(
    llvm::StringRef Filename, std::optional<SeverityLevel> Level,
    const std::optional<std::string> &Category, bool Append) {
  using namespace logger;
  LoggingEnabled = true;
  if (Category.has_value()) {
    CategoriesToStreamVariant[*Category][Level] = Filename.str();
  } else {
    LevelsToStreamVariant[Level] = Filename.str();
  }

  LogFilterLevel = std::min(LogFilterLevel, Level.value_or(CRITICAL));

  std::error_code EC;
  auto [It, Inserted] = [&] {
    if (Append) {
      return LogfileStreams.try_emplace(
          Filename, Filename, EC,
          llvm::sys::fs::OpenFlags::OF_Append |
              llvm::sys::fs::OpenFlags::OF_ChildInherit);
    }

    return LogfileStreams.try_emplace(
        Filename, Filename, EC, llvm::sys::fs::OpenFlags::OF_ChildInherit);
  }();

  if (!Inserted) {
    return true;
  }

  if (EC) {
    LogfileStreams.erase(Filename);
    llvm::errs() << "Failed to open logfile: " << Filename << '\n';
    llvm::errs() << EC.message() << '\n';
    return false;
  }
  return true;
}

llvm::raw_ostream &
Logger::getLogStream(std::optional<SeverityLevel> Level,
                     const std::optional<llvm::StringRef> &Category) {
  using namespace logger;
  if (Category.has_value()) {
    auto CategoryLookupIt = CategoriesToStreamVariant.find(*Category);
    if (CategoryLookupIt == CategoriesToStreamVariant.end()) {
      return llvm::nulls();
    }
    return logger::getLogStream(Level, CategoryLookupIt->second);
  }
  return logger::getLogStream(Level, LevelsToStreamVariant);
}

bool Logger::logCategory(llvm::StringRef Category,
                         std::optional<SeverityLevel> Level) noexcept {
  using namespace logger;
  auto CategoryLookupIt = CategoriesToStreamVariant.find(Category);
  if (CategoryLookupIt == CategoriesToStreamVariant.end()) {
    return false;
  }
  if (Level.has_value()) {
    for (const auto &[LevelThreshold, Stream] : CategoryLookupIt->second) {
      if (LevelThreshold <= *Level) {
        return true;
      }
    }
    return false;
  }
  return CategoryLookupIt->second.count(Level) > 0;
}

void Logger::addLinePrefix(llvm::raw_ostream &OS,
                           std::optional<SeverityLevel> Level,
                           const std::optional<std::string> &Category) {
  // const auto NowTime = std::chrono::steady_clock::now();
  // const auto MillisecondsDuration =
  //     chrono::duration_cast<chrono::milliseconds>(NowTime -
  //     StartTime).count();
  // OS << MillisecondsDuration;
  if (Level.has_value()) {
    OS << '[' << to_string(Level.value()) << ']';
  } // else {
    // OS << ' ';
    // }
  if (Category.has_value()) {
    OS << '[' << Category.value() << ']';
  } // else {
  //   OS << ' ';
  // }
  OS << ' ';
}

} // namespace psr

void psr::initializeLogger(bool UseLogger, const std::string &LogFile) {
  if (!UseLogger) {
    Logger::disable();
    return;
  }
  if (LogFile.empty()) {
    Logger::initializeStderrLogger(Logger::getLoggerFilterLevel());
  } else {
    std::ignore =
        Logger::initializeFileLogger(LogFile, Logger::getLoggerFilterLevel());
  }
}
