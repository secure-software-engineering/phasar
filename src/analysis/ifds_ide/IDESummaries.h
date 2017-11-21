#ifndef IDESUMMARIES_H_
#define IDESUMMARIES_H_

#include "../../utils/Table.h"
#include <iostream>
#include <memory>
using namespace std;

template <class N, class D, class M, class V, class I> class IDESummaries {
private:
  Table<N, D, Table<N, D, shared_ptr<EdgeFunction<V>>>> summaries;

public:
  void addSummary();
  void getSummary();
};

#endif
