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
  template <class K, class V>
class MonoCache {
  // TODO
public:
        unordered_map<K, V> items;
        vector<K> time; //The most recently added or recently used key is placed last
        int c;

        MonoCache(int capacity) {
                c = capacity;
        }

void get(K) {
        unordered_map<K, V>::iterator iter = items.find(K);
        if(iter == items.end())
        return -1; //not found
        else {
                vector<K>::iterator it = std::find(time.begin(), time.end(), K);
                time.erase(it); //The visited element is placed at the end
                time.push_back(K);
                return iter->second; //value
        }

}


void put(K, V) {
        if(items.count(K)) //key already exists in cache
        {
            items[K] = value; //key is set to the new value
            vector<K>::iterator it = std::find(time.begin(), time.end(), K);
            time.erase(it); //Update key usage time
            time.push_back(K);
        }
        else { //Key is not in cache
                if(time.size()==c) { //The cache is full, delete an element that has not been used for the longest time
                        int pop = time[0];
                        time.erase(time.begin()); //Delete the oldest unused element and put it at the beginning of time
                        items.erase(pop);
            }
            time.push_back(K);
            items[K] = value;
        }
    }
};
}
#endif