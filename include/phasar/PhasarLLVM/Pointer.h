/******************************************************************************
 * Copyright (c) 2023 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_POINTER_H
#define PHASAR_PHASARLLVM_POINTER_H

#include "phasar/Config/phasar-config.h" // for PHASAR_USE_SVF
#include "phasar/PhasarLLVM/Pointer/AliasAnalysisView.h"
#include "phasar/PhasarLLVM/Pointer/AndersenOTFAA.h"
#include "phasar/PhasarLLVM/Pointer/FilteredLLVMAliasSet.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/Pointer/LLVMGlobalInitCache.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointerSemantics.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToUtils.h"
#include "phasar/PhasarLLVM/Pointer/LLVMUnionFindAA.h"
#include "phasar/PhasarLLVM/Pointer/LLVMUnionFindAliasSet.h"
#include "phasar/PhasarLLVM/Pointer/MemSSAUtils.h"

#ifdef PHASAR_USE_SVF
#include "phasar/PhasarLLVM/Pointer/SVF/SVFPointsToSet.h"
#endif

#endif // PHASAR_PHASARLLVM_POINTER_H
