module;

#include "phasar/DataFlow/IfdsIde/Solver/IFDSSolver.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDELinearConstantAnalysis.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IFDSSolverTest.h"

export module phasar.dataflow_ifdside_solver;

export namespace psr {
using psr::IDELinearConstantAnalysis;
using psr::IFDSSolver;
using psr::IFDSSolverTest;
using psr::solveIDEProblem;
} // namespace psr
