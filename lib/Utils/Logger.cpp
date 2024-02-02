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

#include "phasar/Utils/TypeTraits.h"

#include "llvm/ADT/STLExtras.h"
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

struct StdOut {};
struct StdErr {};
using StreamVariantTy = std::variant<StdOut, StdErr, std::string>;

static llvm::StringMap<std::map<std::optional<SeverityLevel>, StreamVariantTy>>
    CategoriesToStreamVariant;
static std::map<std::optional<SeverityLevel>, StreamVariantTy>
    LevelsToStreamVariant;

static llvm::StringMap<llvm::raw_fd_ostream> LogfileStreams;
// static inline auto StartTime = std::chrono::steady_clock::now();

// ---

[[nodiscard]] static llvm::raw_ostream &
getLogStreamFromStreamVariant(const StreamVariantTy &StreamVariant) {
  switch (StreamVariant.index()) {
  case variant_idx<StreamVariantTy, StdOut>:
    return llvm::outs();
  case variant_idx<StreamVariantTy, StdErr>:
    return llvm::errs();
  case variant_idx<StreamVariantTy, std::string>: {
    const auto &Filename = std::get<std::string>(StreamVariant);
    auto It = LogfileStreams.find(Filename);
    assert(It != LogfileStreams.end());
    return It->second;
  }
  }
  llvm_unreachable("All stream variants should be handled in the switch above");
}

[[nodiscard]] static llvm::raw_ostream &
getLogStream(std::optional<SeverityLevel> Level,
             const std::map<std::optional<SeverityLevel>, StreamVariantTy>
                 &PassedLevelsToStreamVariant) {
  if (Level.has_value()) {
    for (const auto &[LevelThreshold, StreamVar] :
         llvm::reverse(PassedLevelsToStreamVariant)) {
      if (LevelThreshold <= *Level) {
        return getLogStreamFromStreamVariant(StreamVar);
      }
    }
    // fallthrough
  }

  auto StreamVariantIt = PassedLevelsToStreamVariant.find(std::nullopt);
  if (StreamVariantIt != PassedLevelsToStreamVariant.end()) {
    return getLogStreamFromStreamVariant(StreamVariantIt->second);
  }
  return llvm::nulls();
}

template <typename StdStreamTy>
void initializeLoggerImpl(std::optional<SeverityLevel> Level,
                          const std::optional<std::string> &Category,
                          StdStreamTy Stream) {
  using namespace logger;
  if (Category.has_value()) {
    CategoriesToStreamVariant[*Category].insert_or_assign(Level,
                                                          std::move(Stream));
  } else {
    LevelsToStreamVariant.insert_or_assign(Level, std::move(Stream));
  }
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
  logger::initializeLoggerImpl(Level, Category, logger::StdOut{});
  LogFilterLevel = std::min(LogFilterLevel, Level.value_or(CRITICAL));
}

void Logger::initializeStderrLogger(
    std::optional<SeverityLevel> Level,
    const std::optional<std::string> &Category) {
  LoggingEnabled = true;
  logger::initializeLoggerImpl(Level, Category, logger::StdErr{});
  LogFilterLevel = std::min(LogFilterLevel, Level.value_or(CRITICAL));
}

bool Logger::initializeFileLogger(llvm::StringRef Filename,
                                  std::optional<SeverityLevel> Level,
                                  const std::optional<std::string> &Category,
                                  bool Append) {
  using logger::LogfileStreams;

  LoggingEnabled = true;
  logger::initializeLoggerImpl(Level, Category, Filename.str());
  LogFilterLevel = std::min(LogFilterLevel, Level.value_or(CRITICAL));

  auto Flags = llvm::sys::fs::OpenFlags::OF_ChildInherit;
  if (Append) {
    Flags |= llvm::sys::fs::OpenFlags::OF_Append;
  }

  std::error_code EC;
  LogfileStreams.try_emplace(Filename, Filename, EC, Flags);

  // EC can only be true, if a new filestream was inserted
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

llvm::raw_ostream &Logger::getLogStreamWithLinePrefix(
    std::optional<SeverityLevel> Level,
    const std::optional<llvm::StringRef> &Category) {
  auto &OS = getLogStream(Level, Category);
  addLinePrefix(OS, Level, Category);
  return OS;
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
  return CategoryLookupIt->second.count(Level);
}

void Logger::addLinePrefix(llvm::raw_ostream &OS,
                           std::optional<SeverityLevel> Level,
                           const std::optional<llvm::StringRef> &Category) {
  // const auto NowTime = std::chrono::steady_clock::now();
  // const auto MillisecondsDuration =
  //     chrono::duration_cast<chrono::milliseconds>(NowTime -
  //     StartTime).count();
  // OS << MillisecondsDuration;
  if (Level.has_value()) {
    OS << '[' << to_string(*Level) << ']';
  } // else {
    // OS << ' ';
    // }
  if (Category.has_value()) {
    OS << '[' << *Category << ']';
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
