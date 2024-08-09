/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_CONTROLLER_ANALYSISCONTROLLEREMITTEROPTIONS_H
#define PHASAR_CONTROLLER_ANALYSISCONTROLLEREMITTEROPTIONS_H

namespace psr {
enum class AnalysisControllerEmitterOptions {
  None = 0,
  EmitIR = (1 << 0),
  EmitRawResults = (1 << 1),
  EmitTextReport = (1 << 2),
  EmitGraphicalReport = (1 << 3),
  EmitESGAsDot = (1 << 4),
  EmitTHAsText = (1 << 5),
  EmitTHAsDot = (1 << 6),
  EmitTHAsJson = (1 << 7),
  EmitCGAsText = (1 << 8),
  EmitCGAsDot = (1 << 9),
  EmitCGAsJson = (1 << 10),
  EmitPTAAsText = (1 << 11),
  EmitPTAAsDot = (1 << 12),
  EmitPTAAsJson = (1 << 13),
  EmitStatisticsAsText = (1 << 14),
  EmitStatisticsAsJson = (1 << 15),
};
} // namespace psr

#endif // PHASAR_CONTROLLER_ANALYSISCONTROLLEREMITTEROPTIONS_H
