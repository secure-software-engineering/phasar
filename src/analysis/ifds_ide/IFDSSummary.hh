/*
 * Summary.hh
 *
 *  Created on: 26.05.2017
 *      Author: philipp
 */

#ifndef SRC_ANALYSIS_IFDS_IDE_IFDSSUMMARY_HH_
#define SRC_ANALYSIS_IFDS_IDE_IFDSSUMMARY_HH_

#include <string>
#include <vector>
#include <llvm/IR/Instruction.h>
#include "ZeroValue.hh"
#include "FlowFunction.hh"
using namespace std;

template<typename D>
class IFDSSummary : FlowFunction<D> {
private:
	const llvm::Instruction* StartNode;
	const llvm::Instruction* EndNode;
	D ZeroValue;
	set<D> Outputs;

public:
	IFDSSummary(const llvm::Instruction* Start, const llvm::Instruction* End, D ZV);
	virtual ~IFDSSummary();
	set<D> computeTargets(D source) override {
		if (source == ZeroValue) {
			Outputs.insert(source);
			return Outputs;
		} else {
			return { source };
		}
	}
};

#endif /* SRC_ANALYSIS_IFDS_IDE_IFDSSUMMARY_HH_ */
