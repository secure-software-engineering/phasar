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
#include<list>

namespace psr {
  template <class K, class V>
class MonoCache {
  // TODO
private:
        list< pair<K,V> > items;   //Doubly linked list to store the elements
        unordered_map<K, decltype(items.begin()) > item_map;   //Hashtable for storing the reference
        size_t cache_size;   

private:
        void clean(void){
                while(item_map.size()>cache_size){
                        auto last_ele = items.end(); last_ele --;
                        item_map.erase(last_ele->first);
                        items.pop_back();
                }
        };
public:
        MonoCache(int cache_size_):cache_size(cache_size_){
                ;
        };

        void put(const K &key, const V &val){
                auto ele = item_map.find(key);
                if(ele != item_map.end()){
                        items.erase(ele->second);
                        item_map.erase(ele);
                }
                items.push_front(make_pair(key,val));
                item_map.insert(make_pair(key, items.begin()));
                clean();
        };
        bool exist(const K &key){
                return (item_map.count(key)>0);
        };
        V get(const K &key){
                assert(exist(key));
                auto ele = item_map.find(key);
                items.splice(items.begin(), items, ele->second);
                return ele->second->second;
        };

};

};

#endif
