/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * Table.h
 *
 *  Created on: 07.11.2016
 *      Author: pdschbrt
 */

#ifndef PHASAR_UTILS_TABLE_H_
#define PHASAR_UTILS_TABLE_H_

#include <algorithm>
#include <set>
#include <tuple>
#include <unordered_map>
#include <vector>

// we may wish to replace this by boost::multi_index at some point

namespace psr {

template <typename R, typename C, typename V> class Table {
private:
  std::unordered_map<R, std::unordered_map<C, V>> Tab;

public:
  struct Cell {
    Cell() = default;
    Cell(R Row, C Col, const V Val)
        : Row(Row), Column(Col), Val(std::move(Val)) {}
    ~Cell() = default;
    Cell(const Cell &) = default;
    Cell &operator=(const Cell &) = default;
    Cell(Cell &&) noexcept = default;
    Cell &operator=(Cell &&) noexcept = default;

    [[nodiscard]] R getRowKey() const { return Row; }
    [[nodiscard]] C getColumnKey() const { return Column; }
    [[nodiscard]] V getValue() const { return Val; }

    friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                         const Cell &Cell) {
      return OS << "Cell: " << Cell.r << ", " << Cell.c << ", " << Cell.v;
    }
    friend bool operator<(const Cell &Lhs, const Cell &Rhs) {
      return std::tie(Lhs.Row, Lhs.Column, Lhs.Val) <
             std::tie(Rhs.Row, Rhs.Column, Rhs.Val);
    }
    friend bool operator==(const Cell &Lhs, const Cell &Rhs) {
      return std::tie(Lhs.Row, Lhs.Column, Lhs.Val) ==
             std::tie(Rhs.Row, Rhs.Column, Rhs.Val);
    }

  private:
    R Row;
    C Column;
    V Val;
  };

  Table() = default;
  Table(const Table &T) = default;
  Table &operator=(const Table &T) = default;
  Table(Table &&T) noexcept = default;
  Table &operator=(Table &&T) noexcept = default;
  ~Table() = default;

  void insert(R Row, C Column, V Val) {
    // Associates the specified value with the specified keys.
    Tab[Row][Column] = std::move(Val);
  }

  void insert(const Table &T) { Tab.insert(T.table.begin(), T.table.end()); }

  void clear() { Tab.clear(); }

  [[nodiscard]] bool empty() const { return Tab.empty(); }

  [[nodiscard]] size_t size() const { return Tab.size(); }

  [[nodiscard]] std::set<Cell> cellSet() const {
    // Returns a set of all row key / column key / value triplets.
    std::set<Cell> Result;
    for (const auto &M1 : Tab) {
      for (const auto &M2 : M1.second) {
        Result.emplace(M1.first, M2.first, M2.second);
      }
    }
    return Result;
  }

  [[nodiscard]] std::vector<Cell> cellVec() const {
    // Returns a vector of all row key / column key / value triplets.
    std::vector<Cell> Result;
    for (const auto &M1 : Tab) {
      for (const auto &M2 : M1.second) {
        Result.emplace_back(M1.first, M2.first, M2.second);
      }
    }
    return Result;
  }

  [[nodiscard]] std::unordered_map<R, V> column(C ColumnKey) const {
    // Returns a view of all mappings that have the given column key.
    std::unordered_map<R, V> Column;
    for (const auto &Row : Tab) {
      if (Row.second.count(ColumnKey)) {
        Column[Row.first] = Row.second[ColumnKey];
      }
    }
    return Column;
  }

  [[nodiscard]] std::multiset<C> columnKeySet() const {
    // Returns a set of column keys that have one or more values in the table.
    std::multiset<C> Result;
    for (const auto &M1 : Tab) {
      for (const auto &M2 : M1.second) {
        Result.insert(M2.first);
      }
    }
    return Result;
  }

  [[nodiscard]] std::unordered_map<C, std::unordered_map<R, V>>
  columnMap() const {
    // Returns a view that associates each column key with the corresponding map
    // from row keys to values.
    std::unordered_map<C, std::unordered_map<R, V>> Result;
    for (const auto &M1 : Tab) {
      for (const auto &M2 : Tab.second) {
        Result[M2.first][M1.first] = M2.second;
      }
    }
    return Result;
  }

  [[nodiscard]] bool contains(R RowKey, C ColumnKey) const {
    // Returns true if the table contains a mapping with the specified row and
    // column keys.
    if (auto RowIter = Tab.find(RowKey); RowIter != Tab.end()) {
      return RowIter->second.find(ColumnKey) != RowIter->second.end();
    }
    return false;
  }

  [[nodiscard]] bool containsColumn(C ColumnKey) const {
    // Returns true if the table contains a mapping with the specified column.
    for (const auto &M1 : Tab) {
      if (M1.second.count(ColumnKey)) {
        return true;
      }
    }
    return false;
  }

  [[nodiscard]] bool containsRow(R RowKey) const {
    // Returns true if the table contains a mapping with the specified row key.
    return Tab.count(RowKey);
  }

  [[nodiscard]] bool containsValue(const V &Value) const {
    // Returns true if the table contains a mapping with the specified value.
    for (const auto &M1 : Tab) {
      for (const auto &M2 : M1.second) {
        if (Value == M2.second) {
          return true;
        }
      }
    }
    return false;
  }

  [[nodiscard]] V &get(R RowKey, C ColumnKey) {
    // Returns the value corresponding to the given row and column keys, or null
    // if no such mapping exists.
    return Tab[RowKey][ColumnKey];
  }

  V remove(R RowKey, C ColumnKey) {
    // Removes the mapping, if any, associated with the given keys.
    V Val = Tab[RowKey][ColumnKey];
    Tab[RowKey].erase(ColumnKey);
    return Val;
  }

  void remove(R RowKey) { Tab.erase(RowKey); }

  [[nodiscard]] std::unordered_map<C, V> &row(R RowKey) {
    // Returns a view of all mappings that have the given row key.
    return Tab[RowKey];
  }

  [[nodiscard]] std::multiset<R> rowKeySet() const {
    // Returns a set of row keys that have one or more values in the table.
    std::multiset<R> Result;
    for (const auto &M1 : Tab) {
      Result.insert(M1.first);
    }
    return Result;
  }

  [[nodiscard]] std::unordered_map<R, std::unordered_map<C, V>> rowMap() const {
    // Returns a view that associates each row key with the corresponding map
    // from column keys to values.
    return Tab;
  }

  [[nodiscard]] std::multiset<V> values() const {
    // Returns a collection of all values, which may contain duplicates.
    std::multiset<V> Result;
    for (const auto &M1 : Tab) {
      for (const auto &M2 : M1.second) {
        Result.insert(M2.second);
      }
    }
    return Result;
  }

  friend bool operator==(const Table<R, C, V> &Lhs, const Table<R, C, V> &Rhs) {
    return Lhs.table == Rhs.table;
  }

  friend bool operator<(const Table<R, C, V> &Lhs, const Table<R, C, V> &Rhs) {
    return Lhs.table < Rhs.table;
  }

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                       const Table<R, C, V> &Tab) {
    for (const auto &M1 : Tab.table) {
      for (const auto &M2 : M1.second) {
        OS << "< " << M1.first << " , " << M2.first << " , " << M2.second
           << " >\n";
      }
    }
    return OS;
  }
};

} // namespace psr

#endif
