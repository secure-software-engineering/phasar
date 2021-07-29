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
#include <ostream>
#include <set>
#include <tuple>
#include <unordered_map>
#include <vector>

// we may wish to replace this by boost::multi_index at some point

namespace psr {

template <typename RTy, typename CTy, typename VTy> class Table {
private:
  std::unordered_map<RTy, std::unordered_map<CTy, VTy>> Data;

public:
  struct Cell {
    Cell() = default;
    Cell(RTy Row, CTy Col, VTy Val) : R(Row), C(Col), V(Val) {}
    ~Cell() = default;
    Cell(const Cell &) = default;
    Cell &operator=(const Cell &) = default;
    Cell(Cell &&) noexcept = default;
    Cell &operator=(Cell &&) noexcept = default;

    RTy getRowKey() const { return R; }
    CTy getColumnKey() const { return C; }
    VTy getValue() const { return V; }

    friend std::ostream &operator<<(std::ostream &Os, const Cell &Cel) {
      return Os << "Cell: " << Cel.R << ", " << Cel.C << ", " << Cel.V;
    }
    friend bool operator<(const Cell &Lhs, const Cell &Rhs) {
      return std::tie(Lhs.R, Lhs.Ce, Lhs.Va) < std::tie(Rhs.R, Rhs.C, Rhs.V);
    }
    friend bool operator==(const Cell &Lhs, const Cell &Rhs) {
      return std::tie(Lhs.R, Lhs.C, Lhs.V) == std::tie(Rhs.R, Rhs.C, Rhs.V);
    }

  private:
    RTy R;
    CTy C;
    VTy V;
  };

  Table() = default;
  Table(const Table &T) = default;
  Table &operator=(const Table &T) = default;
  Table(Table &&T) noexcept = default;
  Table &operator=(Table &&T) noexcept = default;
  ~Table() = default;

  void insert(RTy Row, CTy Cel, VTy Val) {
    // Associates the specified value with the specified keys.
    Data[Row][Cel] = std::move(Val);
  }

  void insert(const Table &T) { Data.insert(T.Data.begin(), T.Data.end()); }

  void clear() { Data.clear(); }

  [[nodiscard]] bool empty() const { return Data.empty(); }

  [[nodiscard]] size_t size() const { return Data.size(); }

  [[nodiscard]] std::set<Cell> cellSet() const {
    // Returns a set of all row key / column key / value triplets.
    std::set<Cell> S;
    for (const auto &M1 : Data) {
      for (const auto &M2 : M1.second) {
        S.emplace(M1.first, M2.first, M2.second);
      }
    }
    return S;
  }

  [[nodiscard]] std::vector<Cell> cellVec() const {
    // Returns a vector of all row key / column key / value triplets.
    std::vector<Cell> V;
    for (const auto &M1 : Data) {
      for (const auto &M2 : M1.second) {
        V.emplace_back(M1.first, M2.first, M2.second);
      }
    }
    return V;
  }

  [[nodiscard]] std::unordered_map<RTy, VTy> column(CTy ColumnKey) const {
    // Returns a view of all mappings that have the given column key.
    std::unordered_map<RTy, VTy> Column;
    for (const auto &Row : Data) {
      if (Row.second.count(ColumnKey)) {
        Column[Row.first] = Row.second[ColumnKey];
      }
    }
    return Column;
  }

  [[nodiscard]] std::multiset<CTy> columnKeySet() const {
    // Returns a set of column keys that have one or more values in the table.
    std::multiset<CTy> Colkeys;
    for (const auto &M1 : Data) {
      for (const auto &M2 : M1.second) {
        Colkeys.insert(M2.first);
      }
    }
    return Colkeys;
  }

  [[nodiscard]] std::unordered_map<CTy, std::unordered_map<RTy, VTy>>
  columnMap() const {
    // Returns a view that associates each column key with the corresponding map
    // from row keys to values.
    std::unordered_map<CTy, std::unordered_map<RTy, VTy>> Columnmap;
    for (const auto &M1 : Data) {
      for (const auto &M2 : Data.second) {
        Columnmap[M2.first][M1.first] = M2.second;
      }
    }
    return Columnmap;
  }

  [[nodiscard]] bool contains(RTy RowKey, CTy ColumnKey) const {
    // Returns true if the table contains a mapping with the specified row and
    // column keys.
    if (auto RowIter = Data.find(RowKey); RowIter != Data.end()) {
      return RowIter->second.find(ColumnKey) != RowIter->second.end();
    }
    return false;
  }

  [[nodiscard]] bool containsColumn(CTy ColumnKey) const {
    // Returns true if the table contains a mapping with the specified column.
    for (const auto &M1 : Data) {
      if (M1.second.count(ColumnKey)) {
        return true;
      }
    }
    return false;
  }

  [[nodiscard]] bool containsRow(RTy RowKey) const {
    // Returns true if the table contains a mapping with the specified row key.
    return Data.count(RowKey);
  }

  [[nodiscard]] bool containsValue(const VTy &Value) const {
    // Returns true if the table contains a mapping with the specified value.
    for (const auto &M1 : Data) {
      for (const auto &M2 : M1.second) {
        if (Value == M2.second) {
          return true;
        }
      }
    }
    return false;
  }

  [[nodiscard]] VTy &get(RTy RowKey, CTy ColumnKey) {
    // Returns the value corresponding to the given row and column keys, or null
    // if no such mapping exists.
    return Data[RowKey][ColumnKey];
  }

  VTy remove(RTy RowKey, CTy ColumnKey) {
    // Removes the mapping, if any, associated with the given keys.
    VTy Val = Data[RowKey][ColumnKey];
    Data[RowKey].erase(ColumnKey);
    return Val;
  }

  void remove(RTy RowKey) { Data.erase(RowKey); }

  [[nodiscard]] std::unordered_map<CTy, VTy> &row(RTy RowKey) {
    // Returns a view of all mappings that have the given row key.
    return Data[RowKey];
  }

  [[nodiscard]] std::multiset<RTy> rowKeySet() const {
    // Returns a set of row keys that have one or more values in the table.
    std::multiset<RTy> S;
    for (const auto &M1 : Data) {
      S.insert(M1.first);
    }
    return S;
  }

  [[nodiscard]] std::unordered_map<RTy, std::unordered_map<CTy, VTy>> rowMap() const {
    // Returns a view that associates each row key with the corresponding map
    // from column keys to values.
    return Data;
  }

  [[nodiscard]] std::multiset<VTy> values() const {
    // Returns a collection of all values, which may contain duplicates.
    std::multiset<VTy> S;
    for (const auto &M1 : Data) {
      for (const auto &M2 : M1.second) {
        S.insert(M2.second);
      }
    }
    return S;
  }

  friend bool operator==(const Table<RTy, CTy, VTy> &Lhs, const Table<RTy, CTy, VTy> &Rhs) {
    return Lhs.Data == Rhs.Data;
  }

  friend bool operator<(const Table<RTy, CTy, VTy> &Lhs, const Table<RTy, CTy, VTy> &Rhs) {
    return Lhs.Data < Rhs.Data;
  }

  friend std::ostream &operator<<(std::ostream &Os, const Table<RTy, CTy, VTy> &T) {
    for (const auto &M1 : T.Data) {
      for (const auto &M2 : M1.second) {
        Os << "< " << M1.first << " , " << M2.first << " , " << M2.second
           << " >\n";
      }
    }
    return Os;
  }
};

} // namespace psr

#endif
