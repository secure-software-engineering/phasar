/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * PAMMMacros.h
 *
 *  Created on: 02.10.2018
 *      Author: rleer
 */

#ifndef PHASAR_UTILS_PAMMMACROS_H_
#define PHASAR_UTILS_PAMMMACROS_H_

#include "phasar/Config/phasar-config.h"
#include "phasar/Utils/Macros.h"
#include "phasar/Utils/PAMM.h"

#define PAMM_CATEGORY(NAME, ...)                                               \
  static inline ::psr::pamm::Category<> PAMMCategory {                         \
    #NAME __VA_OPT__(, ) __VA_ARGS__                                           \
  }

#define PAMM_COUNTER(COUNTER_ID, SEV_LVL)                                      \
  static inline ::psr::pamm::Counter<PAMM_CURR_SEV_LEVEL >=                    \
                                         PAMM_SEVERITY_LEVEL::SEV_LVL,         \
                                     #COUNTER_ID, &PAMMCategory>               \
      COUNTER_ID

#define PAMM_MINMAX_COUNTER(COUNTER_ID, SEV_LVL)                               \
  static inline ::psr::pamm::MinMaxCounter<PAMM_CURR_SEV_LEVEL >=              \
                                               PAMM_SEVERITY_LEVEL::SEV_LVL,   \
                                           #COUNTER_ID, &PAMMCategory>         \
      COUNTER_ID

#define PAMM_HISTOGRAM(HISTOGRAM_ID, SEV_LVL)                                  \
  static inline ::psr::pamm::Histogram<PAMM_CURR_SEV_LEVEL >=                  \
                                           PAMM_SEVERITY_LEVEL::SEV_LVL,       \
                                       #HISTOGRAM_ID, &PAMMCategory>           \
      HISTOGRAM_ID

#define PAMM_TIMER(TIMER_ID, SEV_LVL)                                          \
  static inline ::psr::pamm::Timer<PAMM_CURR_SEV_LEVEL >=                      \
                                       PAMM_SEVERITY_LEVEL::SEV_LVL,           \
                                   #TIMER_ID, &PAMMCategory>                   \
      TIMER_ID

#define PAMM_SCOPED_TIMER(TIMER_ID)                                            \
  [[maybe_unused]] ::psr::pamm::ScopedTimer PSR_CONCAT(PAMMScopedTimer,        \
                                                       __COUNTER__) {          \
    TIMER_ID                                                                   \
  }

#if defined(PAMM_FULL) || defined(PAMM_CORE)

#define PAMM_GET_INSTANCE PAMM &pamm = PAMM::getInstance()
#define PAMM_RESET pamm.reset()

#define START_TIMER(TIMER_ID, SEV_LVL)                                         \
  if constexpr (PAMM_CURR_SEV_LEVEL >= PAMM_SEVERITY_LEVEL::SEV_LVL) {         \
    pamm.startTimer(TIMER_ID);                                                 \
  }
#define RESET_TIMER(TIMER_ID, SEV_LVL)                                         \
  if constexpr (PAMM_CURR_SEV_LEVEL >= PAMM_SEVERITY_LEVEL::SEV_LVL) {         \
    pamm.resetTimer(TIMER_ID);                                                 \
  }
#define PAUSE_TIMER(TIMER_ID, SEV_LVL)                                         \
  if constexpr (PAMM_CURR_SEV_LEVEL >= PAMM_SEVERITY_LEVEL::SEV_LVL) {         \
    pamm.stopTimer(TIMER_ID, true);                                            \
  }
#define STOP_TIMER(TIMER_ID, SEV_LVL)                                          \
  if constexpr (PAMM_CURR_SEV_LEVEL >= PAMM_SEVERITY_LEVEL::SEV_LVL) {         \
    pamm.stopTimer(TIMER_ID);                                                  \
  }
#define PRINT_TIMER(TIMER_ID)                                                  \
  pamm.getPrintableDuration(pamm.elapsedTime(TIMER_ID))

#define GET_SUM_COUNT(...) pamm.getSumCount(__VA_ARGS__)

#define PRINT_MEASURED_DATA(OUTPUT_STREAM) pamm.printMeasuredData(OUTPUT_STREAM)
#define EXPORT_MEASURED_DATA(PATH) pamm.exportMeasuredData(PATH)

#else
#define PAMM_GET_INSTANCE
#define PAMM_RESET
#define START_TIMER(TIMER_ID, SEV_LVL)
#define RESET_TIMER(TIMER_ID, SEV_LVL)
#define PAUSE_TIMER(TIMER_ID, SEV_LVL)
#define STOP_TIMER(TIMER_ID, SEV_LVL)
#define PRINT_MEASURED_DATA(OUTPUT_STREAM)
#define EXPORT_MEASURED_DATA(PATH)
// The following macros could be used in log messages, thus they have to
// provide some default value to avoid compiler errors
#define PRINT_TIMER(TIMER_ID) "<none>"
#define GET_SUM_COUNT(...) "<none>"

#endif

#endif
