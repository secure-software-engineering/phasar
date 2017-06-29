#include "ObservedCallingContexts.hh"

void ObservedCallingContexts::addObservedCTX(string FName, vector<bool> CTX) {
	ObservedCTX[FName].insert(CTX);
}

bool ObservedCallingContexts::containsCTX(string FName) {
	return ObservedCTX.find(FName) != ObservedCTX.end();
}

set<vector<bool>> ObservedCallingContexts::getObservedCTX(string FName) {
	return ObservedCTX[FName];
}

void ObservedCallingContexts::print() {
	for (auto& entry : ObservedCTX) {
		cout << entry.first << "\n";
		for (auto& ctx : entry.second) {
			for_each(ctx.begin(), ctx.end(), [](bool b) {
				cout << b;
			});
		}
	}
}
