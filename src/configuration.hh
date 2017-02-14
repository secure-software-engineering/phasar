#ifndef CONFIGURATION_HH
#define CONFIGURATION_HH

#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>
#include <boost/container/flat_set.hpp>
#include <boost/container/flat_map.hpp>

template<typename T>
using Set = std::set<T>;

template<typename T, typename U>
using Map = std::map<T, U>;

#endif
