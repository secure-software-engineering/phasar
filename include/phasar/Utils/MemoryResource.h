/// On some MAC systems, <memory_resource> is still not fully implemented, so do
/// a workaround here

#ifndef PHASAR_UTILS_MEMORYRESOURCE_H
#if !defined(__has_include) || __has_include(<memory_resource>)
#define PHASAR_UTILS_MEMORYRESOURCE_H 1
#include <memory_resource>
#else
#define PHASAR_UTILS_MEMORYRESOURCE_H 0
#endif
#endif
