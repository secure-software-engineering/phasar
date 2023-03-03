/******************************************************************************
 * Copyright (c) 2018 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "phasar/PhasarPass/Options.h"

#include "llvm/Support/CommandLine.h"

#include <string>

using namespace psr;
using namespace std;

namespace cl = llvm::cl;

cl::OptionCategory PhASARCategory("PhASAR Options",
                                  "Configure the PhASAR framework");

std::string psr::DataFlowAnalysis;
static cl::opt<std::string, true>
    SetDataFlowAnalysis("data-flow",
                        cl::desc("Set the data-flow analysis to be run"),
                        cl::location(DataFlowAnalysis),
                        cl::init("ifds-solvertest"), cl::cat(PhASARCategory));

std::string psr::PointerAnalysis;
static cl::opt<std::string, true>
    SetPointerAnalysis("pointer",
                       cl::desc("Set the points-to analysis to be run"),
                       cl::location(PointerAnalysis), cl::init("cflsteens"),
                       cl::cat(PhASARCategory));

std::string psr::CallGraphAnalysis;
static cl::opt<std::string, true> SetCallGraphAnalysis(
    "call-graph", cl::desc("Set the call-graph algorithm to be run"),
    cl::location(CallGraphAnalysis), cl::init("OTF"), cl::cat(PhASARCategory));

std::vector<std::string> psr::EntryPoints;
static cl::list<std::string, std::vector<std::string>>
    SetEntryPoints("entry-points", cl::desc("Set the analysis's entry points"),
                   cl::location(EntryPoints), cl::cat(PhASARCategory),
                   cl::CommaSeparated);

std::string psr::PammOutputFile;
static cl::opt<std::string, true>
    SetPammOutputFile("pamm-out", cl::desc("Filename for PAMM's gathered data"),
                      cl::location(PammOutputFile), cl::init("PAMM_data.json"),
                      cl::cat(PhASARCategory));

bool psr::PrintEdgeRecorder;
static cl::opt<bool, true> SetPrintEdgeRecorder(
    "printedgerec", cl::desc("Print the IFDS/IDE edge recorder"),
    cl::location(PrintEdgeRecorder), cl::init(false), cl::cat(PhASARCategory));

bool psr::InitLogger;
static cl::opt<bool, true> SetInitializeLogger(
    "init-logger", cl::desc("Initialize the logger (caution: very expensive)"),
    cl::location(InitLogger), cl::init(false), cl::cat(PhASARCategory));

bool psr::DumpResults;
static cl::opt<bool, true> SetDumpResults(
    "dump-results", cl::desc("Dump the analysis results to stdout"),
    cl::location(DumpResults), cl::init(true), cl::cat(PhASARCategory));
