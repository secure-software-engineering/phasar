/// On some MAC systems, <memory_resource> is still not fully implemented, so do
/// a workaround here

#ifndef HAS_MEMORY_RESOURCE
#if !defined(__has_include) || __has_include(<memory_resource>)
#define HAS_MEMORY_RESOURCE 1
#else
#define HAS_MEMORY_RESOURCE 0
#endif
#endif
