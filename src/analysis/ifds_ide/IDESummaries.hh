#ifndef IDESUMMARIES_HH_
#define IDESUMMARIES_HH_

#include "../../utils/Table.hh"
#include <iostream>
#include <memory>
using namespace std;

template <class N, class D, class M, class V, class I> class IDESummaries {
private:
	Table<N, D, Table<N, D, shared_ptr<EdgeFunction<V>>>> summaries;

public:
	void addSummary();
	void getSummary();

  // bool store();
  // static IDESummaries restore(const string &id);
};

#endif
