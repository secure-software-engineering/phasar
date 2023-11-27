// Make sure, clangd always picks the right source file to infer the
// compile-commands for IDESolverImpl.h. Otherwise this leads to strange eror
// squiggles
#include "phasar/DataFlow/IfdsIde/Solver/detail/IDESolverImpl.h"
