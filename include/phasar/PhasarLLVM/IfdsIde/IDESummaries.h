/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef IDESUMMARIES_H_
#define IDESUMMARIES_H_

#include <iostream>
#include <memory>
#include <phasar/PhasarLLVM/IfdsIde/EdgeFunction.h>
#include <phasar/Utils/Table.h>
using namespace std;
namespace psr{

template <typename N, typename D, typename M, typename V> class IDESummaries {
private:
  Table<N, D, Table<N, D, shared_ptr<EdgeFunction<V>>>> summaries;

public:
  void addSummaries(Table<N, D, Table<N, D, shared_ptr<EdgeFunction<V>>>> Sum) {
    summaries.insert(Sum);
  }
  Table<N, D, Table<N, D, shared_ptr<EdgeFunction<V>>>> getSummaries() {
    return summaries;
  }
};

}//namespace psr

#endif
