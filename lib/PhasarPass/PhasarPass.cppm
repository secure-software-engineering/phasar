module;

#include "phasar/PhasarPass/Options.h"
#include "phasar/PhasarPass/PhasarPass.h"
#include "phasar/PhasarPass/PhasarPrinterPass.h"

export module phasar.phasarpass;

export namespace psr {
using psr::CallGraphAnalysis;
using psr::DataFlowAnalysis;
using psr::DumpResults;
using psr::EntryPoints;
using psr::InitLogger;
using psr::PammOutputFile;
using psr::PhasarPass;
using psr::PhasarPrinterPass;
using psr::PointerAnalysis;
using psr::PrintEdgeRecorder;
} // namespace psr
