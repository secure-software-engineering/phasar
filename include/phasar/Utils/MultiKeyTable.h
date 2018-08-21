/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * MultiKeyTable.h
 *
 *  Created on: 07.02.2017
 *      Author: pdschbrt
 */

#ifndef PHASAR_UTILS_MULTIKEYTABLE_H_
#define PHASAR_UTILS_MULTIKEYTABLE_H_

#include <ostream>
#include <set>
#include <tuple>
#include <unordered_map>

namespace psr {

template <typename R, typename C, typename V> class MultiKeyTable {
private:
  std::unordered_multimap<R, std::unordered_multimap<C, V>> multi_key_table;

public:
  struct Cell {
    R r;
    C c;
    V v;
    Cell(R row, C col, V val) : r(row), c(col), v(val) {}
    R getRowKey() { return r; }
    C getColumnKey() { return c; }
    V getValue() { return v; }
    friend std::ostream &operator<<(std::ostream &os, const Cell &c) {
      return os << "Cell: " << c.r << ", " << c.c << ", " << c.v;
    }
    friend bool operator<(const Cell &lhs, const Cell &rhs) {
      return std::tie(lhs.r, lhs.c, lhs.v) < std::tie(rhs.r, rhs.c, rhs.v);
    }
    friend bool operator==(const Cell &lhs, const Cell &rhs) {
      return std::tie(lhs.r, lhs.c, lhs.v) == std::tie(rhs.r, rhs.c, rhs.v);
    }
  };

  MultiKeyTable() = default;

  ~MultiKeyTable() = default;

  MultiKeyTable(const MultiKeyTable &t) = default;

  MultiKeyTable(MultiKeyTable &&t) = default;

  MultiKeyTable &operator=(const MultiKeyTable &t) = default;

  MultiKeyTable &operator=(MultiKeyTable &&t) = default;

  V insert(R r, C c, V v) {
    // Associates the specified value with the specified keys.
    multi_key_table[r][c] = v;
    return v;
  }

  void clear() { multi_key_table.clear(); }

  bool empty() { return multi_key_table.empty(); }

  size_t size() { return multi_key_table.size(); }

  std::multiset<Cell> cellSet() {
    // Returns a set of all row key / column key / value triplets.
    std::multiset<Cell> s;
    for (auto &m1 : multi_key_table) {
      for (auto &m2 : m1.second) {
        s.insert(Cell(m1.first, m2.first, m2.second));
      }
    }
    return s;
  }

  std::unordered_multimap<R, V> column(C columnKey) {
    // Returns a view of all mappings that have the given column key.
    std::unordered_multimap<R, V> column;
    for (auto &row : multi_key_table) {
      if (row.second.count(columnKey))
        column[row.first] = row.second[columnKey];
    }
    return column;
  }

  std::multiset<C> columnKeySet() {
    // Returns a set of column keys that have one or more values in the
    // table.
    std::multiset<C> colkeys;
    for (auto &m1 : multi_key_table)
      for (auto &m2 : m1.second)
        colkeys.insert(m2.first);
    return colkeys;
  }

  std::unordered_multimap<C, std::unordered_multimap<R, V>> columnMap() {
    // Returns a view that associates each column key with the corresponding
    // map from row keys to values.
    std::unordered_multimap<C, std::unordered_multimap<R, V>> columnmap;
    for (auto &m1 : multi_key_table) {
      for (auto &m2 : multi_key_table.second) {
        columnmap[m2.first][m1.first] = m2.second;
      }
    }
    return columnmap;
  }

  bool contains(R rowKey, C columnKey) {
    // Returns true if the table contains a mapping with the specified row
    // and column keys.
    if (multi_key_table.count(rowKey))
      return multi_key_table[rowKey].count(columnKey);
    return false;
  }

  bool containsColumn(C columnKey) {
    // Returns true if the table contains a mapping with the specified
    // column.
    for (auto &m1 : multi_key_table)
      if (m1.second.count(columnKey))
        return true;
    return false;
  }

  bool containsRow(R rowKey) {
    // Returns true if the table contains a mapping with the specified row
    // key.
    return multi_key_table.count(rowKey);
  }

  bool containsValue(V value) {
    // Returns true if the table contains a mapping with the specified
    // value.
    for (auto &m1 : multi_key_table)
      for (auto &m2 : m1.second)
        if (value == m2.second)
          return true;
    return false;
  }

  V get(R rowKey, C columnKey) {
    // Returns the value corresponding to the given row and column keys, or null
    // if no such mapping exists.
    // if (table.count(rowKey))
    //	if (table[rowKey].count(columnKey))
    return multi_key_table[rowKey][columnKey];
    // return V();
  }

  V remove(R rowKey, C columnKey) {
    // Removes the mapping, if any, associated with the given keys.
    V v;
    if (contains(rowKey, columnKey)) {
      v = multi_key_table[rowKey][columnKey];
      multi_key_table[rowKey].erase(columnKey);
    }
    return v;
  }

  std::unordered_multimap<C, V> &row(R rowKey) {
    // Returns a view of all mappings that have the given row key.
    return multi_key_table[rowKey];
  }

  std::multiset<R> rowKeySet() {
    // Returns a set of row keys that have one or more values in the table.
    std::multiset<R> s;
    for (auto &m1 : multi_key_table)
      s.insert(m1.first);
    return s;
  }

  std::unordered_multimap<R, std::unordered_multimap<C, V>> rowMap() {
    // Returns a view that associates each row key with the corresponding
    // map from column keys to values.
    return multi_key_table;
  }

  std::multiset<V> values() {
    // Returns a collection of all values, which may contain duplicates.
    std::multiset<V> s;
    for (auto &m1 : multi_key_table)
      for (auto &m2 : m1.second)
        s.insert(m2.second);
    return s;
  }

  friend bool operator==(const MultiKeyTable<R, C, V> &lhs,
                         const MultiKeyTable<R, C, V> &rhs) {
    return lhs.multi_key_table == rhs.multi_key_table;
  }

  friend bool operator<(const MultiKeyTable<R, C, V> &lhs,
                        const MultiKeyTable<R, C, V> &rhs) {
    return lhs.multi_key_table < rhs.multi_key_table;
  }

  friend std::ostream &operator<<(std::ostream &os,
                                  const MultiKeyTable<R, C, V> &t) {
    for (auto &m1 : t.multi_key_table)
      for (auto &m2 : m1.second)
        os << "< " << m1.first << " , " << m2.first << " , " << m2.second
           << " >\n";
    return os;
  }
};

} // namespace psr

#endif
