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
 *  Created on: 31.08.2016
 *      Author: pdschbrt
 */

#ifndef PHASAR_UTILS_MULTIINTEXTABLE_H_
#define PHASAR_UTILS_MULTIINTEXTABLE_H_

#include <ostream>
#include <string>

#include "boost/multi_index/composite_key.hpp"
#include "boost/multi_index/hashed_index.hpp"
#include "boost/multi_index/member.hpp"
#include "boost/multi_index/ordered_index.hpp"
#include "boost/multi_index_container.hpp"

namespace psr {

struct ORDERED_ROW_COL_KEY_TAG {};
struct HASHED_ROW_COL_KEY_TAG {};

template <typename R, typename C, typename V> struct MultiIndexTable {
  struct TableData {
    R rowkey;
    C columnkey;
    V value;
    TableData(const R r, const C c, const V v)
        : rowkey(r), columnkey(c), value(v) {}
  };

  struct row_col_key
      : boost::multi_index::composite_key<
            TableData, BOOST_MULTI_INDEX_MEMBER(TableData, R, rowkey),
            BOOST_MULTI_INDEX_MEMBER(TableData, C, columnkey)> {};

  typedef boost::multi_index_container<
      TableData,
      boost::multi_index::indexed_by<
          boost::multi_index::ordered_unique<
              boost::multi_index::tag<ORDERED_ROW_COL_KEY_TAG>, row_col_key>,
          boost::multi_index::hashed_unique<
              boost::multi_index::tag<HASHED_ROW_COL_KEY_TAG>, row_col_key>>>
      InternTable;

  // ordered indices
  typedef typename boost::multi_index::index<
      InternTable, ORDERED_ROW_COL_KEY_TAG>::type ordered_row_col_key_view_t;
  typedef
      typename boost::multi_index::index<InternTable, ORDERED_ROW_COL_KEY_TAG>::
          type::const_iterator ordered_row_col_key_iterator_t;

  // hashed indices
  typedef typename boost::multi_index::index<
      InternTable, HASHED_ROW_COL_KEY_TAG>::type hashed_row_col_key_view_t;
  typedef
      typename boost::multi_index::index<InternTable, HASHED_ROW_COL_KEY_TAG>::
          type::const_iterator hashed_row_col_key_iterator_t;

  // the indexed table containing instances of TableData
  InternTable IndexedTable;

  friend std::ostream &operator<<(std::ostream &os, const InternTable &itab) {
    return os << "error: unsupported operation!";
  }
};

} // namespace psr

#endif
