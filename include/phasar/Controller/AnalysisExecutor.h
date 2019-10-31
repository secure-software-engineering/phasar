/******************************************************************************
 * Copyright (c) 2019 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_CONTROLLER_ANALYSIS_EXECUTOR_H_
#define PHASAR_CONTROLLER_ANALYSIS_EXECUTOR_H_

namespace psr {

class ProjectIRDB;

class AnalysisExecutor {
public:
  void testExecutor(ProjectIRDB &IRDB);
};

} // namespace psr

#endif
