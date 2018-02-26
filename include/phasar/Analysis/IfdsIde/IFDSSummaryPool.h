/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * DynamicSummaries.h
 *
 *  Created on: 08.05.2017
 *      Author: philipp
 */

#ifndef SRC_ANALYSIS_IFDS_IDE_IFDSSUMMARYPOOL_H_
#define SRC_ANALYSIS_IFDS_IDE_IFDSSUMMARYPOOL_H_

#include "../../analysis/ifds_ide/FlowFunction.h"
#include "../../analysis/ifds_ide/IFDSSummary.h"
#include "../../analysis/ifds_ide/flow_func/GenAll.h"
#include "../../config/ContainerConfiguration.h"
#include <algorithm>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
using namespace std;

template <typename D, typename N> class IFDSSummaryPool {
private:
  /// Stores the summary that starts at a given node.
  map<N, map<vector<bool>, IFDSSummary<D, N>>> SummaryMap;

public:
  IFDSSummaryPool() = default;
  ~IFDSSummaryPool() = default;

  void insertSummary(N StartNode, vector<bool> Context,
                     IFDSSummary<D, N> Summary) {
    SummaryMap[StartNode][Context] = Summary;
  }

  bool containsSummary(N StartNode) { return SummaryMap.count(StartNode); }

  set<D> getSummary(N StartNode, vector<bool> Context) {
    return SummaryMap[StartNode][Context];
  }

  void print() {
    cout << "DynamicSummaries:\n";
    for (auto &entry : SummaryMap) {
      cout << "Function: " << entry.first << "\n";
      for (auto &context_summaries : entry.second) {
        cout << "Context: ";
        for_each(context_summaries.first.begin(), context_summaries.first.end(),
                 [](bool b) { cout << b; });
        cout << "\n";
        cout << "Beg results:\n";
        for (auto &result : context_summaries.second) {
          // result->dump();
          cout << "fixme\n";
        }
        cout << "End results!\n";
      }
    }
  }
};

#endif
