/******************************************************************************
 * Copyright (c) 2018 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <string>

#include "llvm/Support/CommandLine.h"

#include "phasar/PhasarPass/Options.h"

using namespace psr;
using namespace std;

llvm::cl::OptionCategory PhASARCategory("PhASAR Options", // NOLINT
                                        "Configure the PhASAR framework");

std::string psr::DataFlowAnalysis;                           // NOLINT
static llvm::cl::opt<std::string, true> SetDataFlowAnalysis( // NOLINT
    "data-flow", llvm::cl::desc("Set the data-flow analysis to be run"),
    llvm::cl::location(DataFlowAnalysis), llvm::cl::init("ifds-solvertest"),
    llvm::cl::cat(PhASARCategory));

std::string psr::PointerAnalysis;                           // NOLINT
static llvm::cl::opt<std::string, true> SetPointerAnalysis( // NOLINT
    "pointer", llvm::cl::desc("Set the points-to analysis to be run"),
    llvm::cl::location(PointerAnalysis), llvm::cl::init("cflsteens"),
    llvm::cl::cat(PhASARCategory));

std::string psr::CallGraphAnalysis;                           // NOLINT
static llvm::cl::opt<std::string, true> SetCallGraphAnalysis( // NOLINT
    "call-graph", llvm::cl::desc("Set the call-graph algorithm to be run"),
    llvm::cl::location(CallGraphAnalysis), llvm::cl::init("OTF"),
    llvm::cl::cat(PhASARCategory));

std::vector<std::string> psr::EntryPoints; // NOLINT
static llvm::cl::list<std::string, std::vector<std::string>>
    SetEntryPoints("entry-points", // NOLINT
                   llvm::cl::desc("Set the analysis's entry points"),
                   llvm::cl::location(EntryPoints),
                   llvm::cl::cat(PhASARCategory), llvm::cl::CommaSeparated);

std::string psr::PammOutputFile;                           // NOLINT
static llvm::cl::opt<std::string, true> SetPammOutputFile( // NOLINT
    "pamm-out", llvm::cl::desc("Filename for PAMM's gathered data"),
    llvm::cl::location(PammOutputFile), llvm::cl::init("PAMM_data.json"),
    llvm::cl::cat(PhASARCategory));

bool psr::PrintEdgeRecorder; // NOLINT
static llvm::cl::opt<bool, true>
    SetPrintEdgeRecorder("printedgerec", // NOLINT
                         llvm::cl::desc("Print the IFDS/IDE edge recorder"),
                         llvm::cl::location(PrintEdgeRecorder),
                         llvm::cl::init(false), llvm::cl::cat(PhASARCategory));

bool psr::InitLogger;                                 // NOLINT
static llvm::cl::opt<bool, true> SetInitializeLogger( // NOLINT
    "init-logger",
    llvm::cl::desc("Initialize the logger (caution: very expensive)"),
    llvm::cl::location(InitLogger), llvm::cl::init(false),
    llvm::cl::cat(PhASARCategory));

bool psr::DumpResults; // NOLINT
static llvm::cl::opt<bool, true>
    SetDumpResults("dump-results", // NOLINT
                   llvm::cl::desc("Dump the analysis results to stdout"),
                   llvm::cl::location(DumpResults), llvm::cl::init(true),
                   llvm::cl::cat(PhASARCategory));
