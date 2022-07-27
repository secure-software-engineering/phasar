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

namespace psr {
/// Defines the different level of severity of PAMM's performance evaluation
enum PAMM_SEVERITY_LEVEL { Off = 0, Core, Full }; // NOLINT

#if defined(PAMM_FULL)
static constexpr unsigned PAMM_CURR_SEV_LEVEL = 2; // NOLINT
#elif defined(PAMM_CORE)
static constexpr unsigned PAMM_CURR_SEV_LEVEL = 1; // NOLINT
#else
static constexpr unsigned PAMM_CURR_SEV_LEVEL = 0; // NOLINT
#endif

} // namespace psr

#if defined(PAMM_FULL) || defined(PAMM_CORE)
// Only include PAMM header if it is used
#include "phasar/Utils/PAMM.h"

#define PAMM_GET_INSTANCE PAMM &pamm = PAMM::getInstance()
#define PAMM_RESET pamm.reset()

#define START_TIMER(TIMER_ID, SEV_LVL)                                         \
  if constexpr (PAMM_CURR_SEV_LEVEL >= SEV_LVL) {                              \
    pamm.startTimer(TIMER_ID);                                                 \
  }
#define RESET_TIMER(TIMER_ID, SEV_LVL)                                         \
  if constexpr (PAMM_CURR_SEV_LEVEL >= SEV_LVL) {                              \
    pamm.resetTimer(TIMER_ID);                                                 \
  }
#define PAUSE_TIMER(TIMER_ID, SEV_LVL)                                         \
  if constexpr (PAMM_CURR_SEV_LEVEL >= SEV_LVL) {                              \
    pamm.stopTimer(TIMER_ID, true);                                            \
  }
#define STOP_TIMER(TIMER_ID, SEV_LVL)                                          \
  if constexpr (PAMM_CURR_SEV_LEVEL >= SEV_LVL) {                              \
    pamm.stopTimer(TIMER_ID);                                                  \
  }
#define PRINT_TIMER(TIMER_ID)                                                  \
  pamm.getPrintableDuration(pamm.elapsedTime(TIMER_ID))

#define REG_COUNTER(COUNTER_ID, INIT_VALUE, SEV_LVL)                           \
  if constexpr (PAMM_CURR_SEV_LEVEL >= SEV_LVL) {                              \
    pamm.regCounter(COUNTER_ID, INIT_VALUE);                                   \
  }
#define INC_COUNTER(COUNTER_ID, VALUE, SEV_LVL)                                \
  if constexpr (PAMM_CURR_SEV_LEVEL >= SEV_LVL) {                              \
    pamm.incCounter(COUNTER_ID, VALUE);                                        \
  }
#define DEC_COUNTER(COUNTER_ID, VALUE, SEV_LVL)                                \
  if constexpr (PAMM_CURR_SEV_LEVEL >= SEV_LVL) {                              \
    pamm.decCounter(COUNTER_ID, VALUE);                                        \
  }
#define GET_COUNTER(COUNTER_ID) pamm.getCounter(COUNTER_ID)
#define GET_SUM_COUNT(...) pamm.getSumCount(__VA_ARGS__)

#define REG_HISTOGRAM(HISTOGRAM_ID, SEV_LVL)                                   \
  if constexpr (PAMM_CURR_SEV_LEVEL >= SEV_LVL) {                              \
    pamm.regHistogram(HISTOGRAM_ID);                                           \
  }
#define ADD_TO_HISTOGRAM(HISTOGRAM_ID, DATAPOINT_ID, DATAPOINT_VALUE, SEV_LVL) \
  if constexpr (PAMM_CURR_SEV_LEVEL >= SEV_LVL) {                              \
    pamm.addToHistogram(HISTOGRAM_ID, std::to_string(DATAPOINT_ID),            \
                        DATAPOINT_VALUE);                                      \
  }

#define PRINT_MEASURED_DATA(OUTPUT_STREAM) pamm.printMeasuredData(OUTPUT_STREAM)
#define EXPORT_MEASURED_DATA(PATH) pamm.exportMeasuredData(PATH)

#else
#define PAMM_GET_INSTANCE
#define PAMM_RESET
#define START_TIMER(TIMER_ID, SEV_LVL)
#define RESET_TIMER(TIMER_ID, SEV_LVL)
#define PAUSE_TIMER(TIMER_ID, SEV_LVL)
#define STOP_TIMER(TIMER_ID, SEV_LVL)
#define REG_COUNTER(COUNTER_ID, INIT_VALUE, SEV_LVL)
#define INC_COUNTER(COUNTER_ID, VALUE, SEV_LVL)
#define DEC_COUNTER(COUNTER_ID, VALUE, SEV_LVL)
#define REG_HISTOGRAM(HISTOGRAM_ID, SEV_LVL)
#define ADD_TO_HISTOGRAM(HISTOGRAM_ID, DATAPOINT_ID, DATAPOINT_VALUE, SEV_LVL)
#define PRINT_MEASURED_DATA(OUTPUT_STREAM)
#define EXPORT_MEASURED_DATA(PATH)
// The following macros could be used in log messages, thus they have to
// provide some default value to avoid compiler errors
#define PRINT_TIMER(TIMER_ID) "-1"
#define GET_COUNTER(COUNTER_ID) "-1"
#define GET_SUM_COUNT(...) "-1"
#endif

#endif
