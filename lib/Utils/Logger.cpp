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

#include "llvm/ADT/StringSwitch.h"
#include "llvm/Support/FileSystem.h"

#include "phasar/Utils/Logger.h"

namespace psr {

SeverityLevel parseSeverityLevel(llvm::StringRef Str) {
  return llvm::StringSwitch<SeverityLevel>(Str)
#define SEVERITY_LEVEL(NAME, TYPE) .Case(NAME, SeverityLevel::TYPE)
#include "phasar/Utils/SeverityLevel.def"
      .Default(SeverityLevel::INVALID);
}

std::string Logger::toString(SeverityLevel Level) {
  switch (Level) {
  default:
#define SEVERITY_LEVEL(NAME, TYPE)                                             \
  case SeverityLevel::TYPE:                                                    \
    return NAME;                                                               \
    break;
#include "phasar/Utils/SeverityLevel.def"
  }
}

void Logger::setLoggerFilterLevel(SeverityLevel Level) {
  LogFilterLevel = Level;
}

SeverityLevel Logger::getLoggerFilterLevel() { return LogFilterLevel; }

bool Logger::isLoggingEnabled() { return LoggingEnabled; }

void Logger::initializeStdoutLogger(
    std::optional<SeverityLevel> Level,
    const std::optional<std::string> &Category) {
  LoggingEnabled = true;
  if (Category.has_value()) {
    CategoriesToStreamVariant[Category.value()][Level] = StdStream::STDOUT;
  } else {
    LevelsToStreamVariant[Level] = StdStream::STDOUT;
  }
}

void Logger::initializeStderrLogger(
    std::optional<SeverityLevel> Level,
    const std::optional<std::string> &Category) {
  LoggingEnabled = true;
  if (Category.has_value()) {
    CategoriesToStreamVariant[Category.value()][Level] = StdStream::STDERR;
  } else {
    LevelsToStreamVariant[Level] = StdStream::STDERR;
  }
}

[[nodiscard]] bool Logger::initializeFileLogger(
    llvm::StringRef Filename, std::optional<SeverityLevel> Level,
    const std::optional<std::string> &Category, bool Append) {
  LoggingEnabled = true;
  if (Category.has_value()) {
    CategoriesToStreamVariant[Category.value()][Level] = Filename.str();
  } else {
    LevelsToStreamVariant[Level] = Filename.str();
  }

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
  if (Category.has_value()) {
    auto CategoryLookupIt = CategoriesToStreamVariant.find(Category.value());
    if (CategoryLookupIt == CategoriesToStreamVariant.end()) {
      return llvm::nulls();
    }
    return getLogStream(Level, CategoryLookupIt->second);
  }
  return getLogStream(Level, LevelsToStreamVariant);
}

llvm::raw_ostream &
Logger::getLogStream(std::optional<SeverityLevel> Level,
                     const std::map<std::optional<SeverityLevel>,
                                    std::variant<StdStream, std::string>>
                         &PassedLevelsToStreamVariant) {
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

llvm::raw_ostream &Logger::getLogStreamFromStreamVariant(
    const std::variant<StdStream, std::string> &StreamVariant) {
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

bool Logger::logCategory(llvm::StringRef Category,
                         std::optional<SeverityLevel> Level) {
  auto CategoryLookupIt = CategoriesToStreamVariant.find(Category);
  if (CategoryLookupIt == CategoriesToStreamVariant.end()) {
    return false;
  }
  if (Level.has_value()) {
    for (const auto &[LevelThreshold, Stream] : CategoryLookupIt->second) {
      if (LevelThreshold <= Level.value()) {
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
    OS << '[' << toString(Level.value()) << ']';
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

void initializeLogger(bool UseLogger, const std::string &LogFile) {
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

} // namespace psr
