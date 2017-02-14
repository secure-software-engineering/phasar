/*
 * LLVMIFDSSolver.hh
 *
 *  Created on: 15.09.2016
 *      Author: pdschbrt
 */

#ifndef ANALYSIS_IFDS_IDE_SOLVER_LLVMIFDSSOLVER_HH_
#define ANALYSIS_IFDS_IDE_SOLVER_LLVMIFDSSOLVER_HH_

#include <map>
#include <algorithm>
#include "../IFDSTabulationProblem.hh"
#include "IFDSSolver.hh"
#include "../icfg/ICFG.hh"
#include "../../../utils/Table.hh"
using namespace std;

template<class D, class I>
class LLVMIFDSSolver : public IFDSSolver<const llvm::Instruction*, D, const llvm::Function*, I> {
private:
	const bool DUMP_RESULTS;

public:
	virtual ~LLVMIFDSSolver() = default;

	LLVMIFDSSolver(IFDSTabulationProblem<const llvm::Instruction*,D,const llvm::Function*,I>& problem, bool dumpResults=false)
					: IFDSSolver<const llvm::Instruction*,D,const llvm::Function*,I>(problem),
					  DUMP_RESULTS(dumpResults) {}

	virtual void solve() override
	{
		// do the solving of the analaysis problem
		IFDSSolver<const llvm::Instruction*,D,const llvm::Function*,I>::solve();
		if (DUMP_RESULTS)
			dumpResults();
	}

	void dumpResults()
	{
		cout << "I am a LLVMIFDSSolver result" << endl;
		cout << "### DUMP RESULTS" << endl;
		// TODO present results in a nicer way than just calling llvm's dump()
		// for the following line have a look at:
		// http://stackoverflow.com/questions/1120833/derived-template-class-access-to-base-class-member-data
		// https://isocpp.org/wiki/faq/templates#nondependent-name-lookup-members
		auto results = this->valtab.cellSet();
		if (results.empty()) {
			cout << "EMPTY" << endl;
		} else {
			vector<typename Table<const llvm::Instruction*, const llvm::Value*, BinaryDomain>::Cell> cells;
			for (auto cell : results) {
				cells.push_back(cell);
			}
			sort(cells.begin(), cells.end(),
					[](typename Table<const llvm::Instruction*, const llvm::Value*, BinaryDomain>::Cell a,
					   typename Table<const llvm::Instruction*, const llvm::Value*, BinaryDomain>::Cell b)
					   {
					   	   return a.r < b.r;
					   });
			const llvm::Instruction* prev = nullptr;
			const llvm::Instruction* curr;
			for (unsigned i = 0; i < cells.size(); ++i) {
				curr = cells[i].r;
				if (prev != curr) {
					prev = curr;
					cout << "--- RESULT AT NODE ---" << endl;
					cout << "N" << endl;
					cells[i].r->dump();
					cout << "of function: ";
					if (const llvm::Instruction* inst = llvm::dyn_cast<llvm::Instruction>(cells[i].r)) {
						cout << inst->getFunction()->getName().str() << endl;
					}
				}
				cout << "D" << endl;
				if (cells[i].c == nullptr)
					cout << "  nullptr" << endl;
				else
					cells[i].c->dump();
				cout << endl;
				cout << "V\n  ";
				cout << cells[i].v << endl;
			}
		}
		cout << "### RESULTS AT LAST STATEMENT OF MAIN" << endl;
		auto resultAtEnd = this->resultsAt(this->icfg.getLastInstructionOf("main"));
		if (resultAtEnd.empty()) {
			cout << "EMPTY" << endl;
		} else {
			for (auto entry : resultAtEnd) {
				cout << "\t--- begin entry ---" << endl;
				entry.first->dump();
				//cout << "from function: " << entry.first->getFunction().getName().str() << endl;
				cout << entry.second << endl;
				cout << "\t--- end entry ---" << endl;
			}
		}
		cout << "### END RESULTS AT LAST STATEMENT OF MAIN" << endl;
	}
};

#endif /* ANALYSIS_IFDS_IDE_SOLVER_LLVMIFDSSOLVER_HH_ */
