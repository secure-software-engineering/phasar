#include "SolverConfiguration.hh"

ostream& operator<< (ostream& os, const SolverConfiguration& sc) {
	return os << "SolverConfiguration:\n"
						<< "\tfollowReturnsPastSeeds: " << sc.followReturnsPastSeeds << "\n"
						<< "\tautoAddZero: " << sc.autoAddZero << "\n"
						<< "\tcomputeValues: " << sc.computeValues << "\n"
						<< "\trecordEdges: " << sc.recordEdges << "\n"
						<< "\tcomputePersistedSummaries: " << sc.computePersistedSummaries;
}
