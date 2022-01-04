/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_IFDSFIELDSENSTAINTANALYSIS_UTILS_LOG_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_IFDSFIELDSENSTAINTANALYSIS_UTILS_LOG_H

#include "llvm/Support/raw_ostream.h"

#define LOG_INFO(x)                                                            \
  do {                                                                         \
    llvm::outs() << "[ENV_TRACE] " << x << "\n"; /*NOLINT*/                    \
                                                                               \
    llvm::outs().flush();                                                      \
  } while (0)

#ifdef DEBUG_BUILD
#define LOG_DEBUG(x) LOG_INFO(x)
#else
#define LOG_DEBUG(x)                                                           \
  do {                                                                         \
  } while (0)
#endif

#endif
