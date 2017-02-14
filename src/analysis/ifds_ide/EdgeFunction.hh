/*
 * AbstractEdgeFunction.hh
 *
 *  Created on: 04.08.2016
 *      Author: pdschbrt
 */

#ifndef ANALYSIS_IFDS_IDE_EDGEFUNCTION_HH_
#define ANALYSIS_IFDS_IDE_EDGEFUNCTION_HH_

#include <iostream>

template<class V>
class EdgeFunction {
public:
	virtual ~EdgeFunction() = default;

	virtual V computeTarget(V source) = 0;

	virtual shared_ptr<EdgeFunction<V>> composeWith(shared_ptr<EdgeFunction<V>> secondFunction) = 0;

	virtual shared_ptr<EdgeFunction<V>> joinWith(shared_ptr<EdgeFunction<V>> otherFunction) = 0;

	virtual bool equalTo(shared_ptr<EdgeFunction<V>> other) = 0;

	virtual void dump() { std::cout << "edge function\n"; }
};

#endif /* ANALYSIS_IFDS_IDE_EDGEFUNCTION_HH_ */
