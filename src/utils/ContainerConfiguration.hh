/*
 * ContainerConfiguration.hh
 *
 *  Created on: 02.05.2017
 *      Author: philipp
 */

#ifndef SRC_UTILS_CONTAINERCONFIGURATION_HH_
#define SRC_UTILS_CONTAINERCONFIGURATION_HH_

#include <set>
#include <unordered_set>
#include <boost/container/flat_set.hpp>
#include <llvm/ADT/SmallSet.h>
#include <llvm/ADT/SetVector.h>
#include <map>
#include <unordered_map>
#include <boost/container/flat_map.hpp>
#include <llvm/ADT/StringMap.h>
#include <llvm/ADT/IndexedMap.h>
#include <llvm/ADT/IntervalMap.h>
#include <llvm/ADT/MapVector.h>
#include <vector>
#include <boost/container/small_vector.hpp>
#include <llvm/ADT/SmallVector.h>
// check if we forgot some more useful container implementations

// define the set implementation to use for the ICFG classes ------------------
template<typename T>
using ICFGSet = boost::container::flat_set<T>;
// ----------------------------------------------------------------------------

// define the set implementation to use for the flow functions ----------------
#define FFSetPreAllocSize 10

template<typename T>
using FFSet = boost::container::small_vector<T, FFSetPreAllocSize>;
// ----------------------------------------------------------------------------

// define the map implementation to use for the special summaries -------------
template<typename T, typename U>
using SSMap = boost::container::flat_map<T, U>;
// ----------------------------------------------------------------------------

// define the map implementation to use for the dynamic summaries -------------
template<typename T, typename U>
using DSMap = boost::container::flat_map<T, U>;
// ----------------------------------------------------------------------------

#endif
