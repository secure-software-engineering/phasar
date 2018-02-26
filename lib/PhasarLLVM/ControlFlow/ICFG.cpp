/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * ICFG.cpp
 *
 *  Created on: 17.08.2016
 *      Author: pdschbrt
 */

#include <phasar/PhasarLLVM/ControlFlow/ICFG.h>

const map<string, CallGraphAnalysisType> StringToCallGraphAnalysisType = {
    {"CHA", CallGraphAnalysisType::CHA},
    {"RTA", CallGraphAnalysisType::RTA},
    {"DTA", CallGraphAnalysisType::DTA},
    {"VTA", CallGraphAnalysisType::VTA},
    {"OTF", CallGraphAnalysisType::OTF}};

const map<CallGraphAnalysisType, string> CallGraphAnalysisTypeToString = {
    {CallGraphAnalysisType::CHA, "CHA"},
    {CallGraphAnalysisType::RTA, "RTA"},
    {CallGraphAnalysisType::DTA, "DTA"},
    {CallGraphAnalysisType::VTA, "VTA"},
    {CallGraphAnalysisType::OTF, "OTF"}};

ostream &operator<<(ostream &os, const CallGraphAnalysisType &CGA) {
  return os << CallGraphAnalysisTypeToString.at(CGA);
}
