/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <phasar/PhasarLLVM/IfdsIde/Solver/IFDSToIDETabulationProblem.h>

const shared_ptr<AllBottom<BinaryDomain>> ALL_BOTTOM =
    make_shared<AllBottom<BinaryDomain>>(BinaryDomain::BOTTOM);
