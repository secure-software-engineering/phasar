/******************************************************************************
 * Copyright (c) 2025 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel, bulletSpace and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_LIBCSUMMARY_H
#define PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_LIBCSUMMARY_H

namespace psr {
namespace library_summary {
class FunctionDataFlowFacts;
} // namespace library_summary

[[nodiscard]] const library_summary::FunctionDataFlowFacts &getLibCSummary();
} // namespace psr

#endif // PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_LIBCSUMMARY_H
