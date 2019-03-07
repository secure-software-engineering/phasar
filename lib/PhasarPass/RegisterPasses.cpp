/******************************************************************************
 * Copyright (c) 2018 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <string>

#include <llvm/Support/CommandLine.h>

#include <phasar/PhasarPass/Options.h>

using namespace psr;
using namespace std;

llvm::cl::OptionCategory PhASARCategory("PhASAR Options",
                                        "Configure the PhASAR framework");

std::string psr::DataFlowAnalysis;
static llvm::cl::opt<std::string, true> SetDataFlowAnalysis(
    "data-flow", llvm::cl::desc("Set the data-flow analysis to be run"),
    llvm::cl::location(DataFlowAnalysis), llvm::cl::init("ifds-solvertest"),
    llvm::cl::cat(PhASARCategory));

std::string psr::PointerAnalysis;
static llvm::cl::opt<std::string, true> SetPointerAnalysis(
    "pointer", llvm::cl::desc("Set the points-to analysis to be run"),
    llvm::cl::location(PointerAnalysis), llvm::cl::init("cflsteens"),
    llvm::cl::cat(PhASARCategory));

std::string psr::CallGraphAnalysis;
static llvm::cl::opt<std::string, true> SetCallGraphAnalysis(
    "call-graph", llvm::cl::desc("Set the call-graph algorithm to be run"),
    llvm::cl::location(CallGraphAnalysis), llvm::cl::init("OTF"),
    llvm::cl::cat(PhASARCategory));

std::vector<std::string> psr::EntryPoints;
static llvm::cl::list<std::string, std::vector<std::string>>
    SetEntryPoints("entry-points",
                   llvm::cl::desc("Set the analysis's entry points"),
                   llvm::cl::location(EntryPoints),
                   llvm::cl::cat(PhASARCategory), llvm::cl::CommaSeparated);

std::string psr::PammOutputFile;
static llvm::cl::opt<std::string, true> SetPammOutputFile(
    "pamm-out", llvm::cl::desc("Filename for PAMM's gathered data"),
    llvm::cl::location(PammOutputFile), llvm::cl::init("PAMM_data.json"),
    llvm::cl::cat(PhASARCategory));

bool psr::PrintEdgeRecorder;
static llvm::cl::opt<bool, true>
    SetPrintEdgeRecorder("printedgerec",
                         llvm::cl::desc("Print the IFDS/IDE edge recorder"),
                         llvm::cl::location(PrintEdgeRecorder),
                         llvm::cl::init(false), llvm::cl::cat(PhASARCategory));

bool psr::InitLogger;
static llvm::cl::opt<bool, true> SetInitializeLogger(
    "init-logger",
    llvm::cl::desc("Initialize the logger (caution: very expensive)"),
    llvm::cl::location(InitLogger), llvm::cl::init(false),
    llvm::cl::cat(PhASARCategory));

bool psr::DumpResults;
static llvm::cl::opt<bool, true>
    SetDumpResults("dump-results",
                   llvm::cl::desc("Dump the analysis results to stdout"),
                   llvm::cl::location(DumpResults), llvm::cl::init(true),
                   llvm::cl::cat(PhASARCategory));
