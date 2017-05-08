/*
 * LLVMIDESolver.hh
 *
 *  Created on: 15.09.2016
 *      Author: pdschbrt
 */

#ifndef ANALYSIS_IFDS_IDE_SOLVER_LLVMIDESOLVER_HH_
#define ANALYSIS_IFDS_IDE_SOLVER_LLVMIDESOLVER_HH_

#include "../IDETabluationProblem.hh"
#include "IDESolver.hh"
#include "../icfg/ICFG.hh"

template <class D, class V, class I>
class LLVMIDESolver : public IDESolver<const llvm::Instruction*,
									   D,
									   const llvm::Function*,
									   V,
									   I> {
private:
	const bool DUMP_RESULTS;

public:
	LLVMIDESolver(IDETabluationProblem<const llvm::Instruction*,D,const llvm::Function*,V,I>& problem, bool dumpResults=false)
			    : IDESolver<const llvm::Instruction*,D,const llvm::Function*,V,I>(problem),
				  DUMP_RESULTS(dumpResults) {}

	virtual ~LLVMIDESolver() = default;

	void solve() override
	{
		IDESolver<const llvm::Instruction*,D,const llvm::Function*,V,I>::solve();
		if (DUMP_RESULTS)
			dumpResults();
	}

	void dumpResults()
	{
		cout << "I am a LLVMIDESolver result" << endl;
		cout << "### DUMP RESULTS" << endl;
		// TODO present results in a nicer way than just calling llvm's dump()
		// for the following line have a look at:
		// http://stackoverflow.com/questions/1120833/derived-template-class-access-to-base-class-member-data
		// https://isocpp.org/wiki/faq/templates#nondependent-name-lookup-members
		auto results = this->valtab.cellSet();
		if (results.empty()) {
			cout << "EMPTY" << endl;
		} else {
			for (auto cell : results) {
				cout << "--- IDE START RESULT RECORD ---" << endl;
				cout << "N" << endl;
				cell.r->dump();
				cout << "D" << endl;
				if (cell.c != nullptr)
					cell.c->dump();
				cout << endl;
				cout << "V\n\t";
				cout << cell.v << endl;
				cout << "--- IDE END RESULT RECORD ---" << endl;
			}
			cout << "### END DUMP RESULTS" << endl;
		}
//		cout << "### IDE RESULTS AT LAST STATEMENT OF MAIN" << endl;
//		auto resultAtEnd = this->resultsAt(this->icfg.getLastInstructionOf("main"));
//		for (auto entry : resultAtEnd) {
//			cout << "\t--- begin entry ---" << endl;
//			entry.first->dump();
//			cout << entry.second << endl;
//			cout << "\t--- end entry ---" << endl;
//		}
//		cout << "### IDE END RESULTS AT LAST STATEMENT OF MAIN" << endl;
	}

	void dumpAllInterPathEdges() {
			cout << "COMPUTED INTER PATH EDGES" << endl;
			auto interpe = this->computedInterPathEdges.cellSet();
			for (auto& cell : interpe) {
				cout << "FROM" << endl;
				cell.r->dump();
				cout << "TO" << endl;
				cell.c->dump();
				cout << "FACTS" << endl;
				for (auto& fact : cell.v) {
					cout << "fact" << endl;
					fact.first->dump();
					cout << "produces" << endl;
					for (auto& out : fact.second) {
						out->dump();
					}
				}
			}
		}

	void dumpAllIntraPathEdges() {
		cout << "COMPUTED INTRA PATH EDGES" << endl;
		auto intrape = this->computedIntraPathEdges.cellSet();
		for (auto& cell : intrape) {
			cout << "FROM" << endl;
			cell.r->dump();
			cout << "TO" << endl;
			cell.c->dump();
			cout << "FACTS" << endl;
			for (auto& fact : cell.v) {
				cout << "fact" << endl;
				fact.first->dump();
				cout << "produces" << endl;
				for (auto& out : fact.second) {
				out->dump();
				}
			}
		}
	}

};

#endif /* ANALYSIS_IFDS_IDE_SOLVER_LLVMIDESOLVER_HH_ */
