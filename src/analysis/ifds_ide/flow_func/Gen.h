/*
 * Gen.h
 *
 *  Created on: 04.08.2016
 *      Author: pdschbrt
 */

#ifndef ANALYSIS_IFDS_IDE_FLOW_FUNC_GEN_H_
#define ANALYSIS_IFDS_IDE_FLOW_FUNC_GEN_H_

#include "../FlowFunction.h"
#include <set>

using namespace std;

template <typename D> class Gen : public FlowFunction<D> {
private:
  D genValue;
  D zeroValue;

public:
  Gen(D genValue, D zeroValue) : genValue(genValue), zeroValue(zeroValue) {}
  virtual ~Gen() = default;
  set<D> computeTargets(D source) override {
    if (source == zeroValue)
      return {source, genValue};
    else
      return {source};
  }
};

#endif /* ANALYSIS_IFDS_IDE_FLOW_FUNC_GEN_HH_ */
