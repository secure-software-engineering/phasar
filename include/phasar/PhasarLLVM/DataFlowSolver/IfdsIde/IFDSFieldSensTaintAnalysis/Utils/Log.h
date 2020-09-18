/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#ifndef LOG_H
#define LOG_H

#include "llvm/Support/raw_ostream.h"

#define LOG_INFO(x)                                                            \
  do {                                                                         \
    llvm::outs() << "[ENV_TRACE] " << x << "\n";                               \
    llvm::outs().flush();                                                      \
  } while (0)

#ifdef DEBUG_BUILD
#define LOG_DEBUG(x) LOG_INFO(x)
#else
#define LOG_DEBUG(x)                                                           \
  do {                                                                         \
  } while (0)
#endif

#endif // LOG_H
