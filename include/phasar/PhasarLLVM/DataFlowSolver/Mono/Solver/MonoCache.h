/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Ajay Subramanya Kudli Prasanna Kumar, Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_MONO_SOLVER_MONOCACHE_H_
#define PHASAR_PHASARLLVM_MONO_SOLVER_MONOCACHE_H_

#include<iostream>
#include<unordered_map>
#include<vector>

namespace psr {
template <class K> class MonoCache {

private:
  std::unordered_map<K, std::pair<K, std::vector<K>::iterator>>
      SummariesCache; // < key, < value, keyIterator>>
  std::vector<K>
      lru; // The most recently added or recently used key is placed last
  int c;

  void updateLRU(
      std::unordered_map<K, std::pair<K, std::vector<K>::iterator>>::iterator
          &it) {
    lru.erase(it->second.second); // erase the key iterator
    lru.push_back(it->first);     // push the key to the back of vector
    it->second.second =
        lru.end(); // update the key iterator to the back of vector
  }

public:
  MonoCache(int capacity) { c = capacity; }

  K getSummary(K key) {
    auto summary = SummariesCache.find(key); // looks for key in cache
    if (summary ==
        SummariesCache.end()) { // if the key does not exist, return -1
      return -1;
    } else { // if the key exists, return value and update LRU cache
      updateLRU(summary);
      return summary->second.first; // returns the value
    }
  }

  void addSummary(K key, K value) {

    auto summary =
        SummariesCache.find(key); // checks if key already exists in cache
    if (summary != SummariesCache.end()) {
      updateLRU(summary);
      SummariesCache[key] = {value,
                             lru.end()}; // key is set to the new value and it
                                         // will be in the end of vector
      return;
    } else { // Key is not in cache, check the capacity and insert the new
             // summary value
      if (SummariesCache.size() ==
          c) { // The cache is full, delete a summary that has not been used for
               // the longest time
        auto pop = lru[0]; // least recently used summary is present in the
                           // front of the vector
        SummariesCache.erase(lru.begin()); // Delete the oldest unused element
        lru.pop(pop);
      }
      lru.push_back(key); // push the key to the back of the vector
      SummariesCache.insert(
          {key,
           {value, lru.end()}}); // insert the key, value and the key iterator
                                 // which is present in the back of the vector
    }
  }

  bool hasSummary(K key) {

    auto summary = SummariesCache.find(key);
    if (summary == SummariesCache.end()) {
      return false;
    } else {
      return true;
    }
  }
};
}
#endif