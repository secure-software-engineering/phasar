/******************************************************************************
 * Copyright (c) 2025 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_UTILS_DEFAULTANALYSISPRINTERSELECTOR_H
#define PHASAR_UTILS_DEFAULTANALYSISPRINTERSELECTOR_H

#include "phasar/Utils/DefaultAnalysisPrinter.h"
#include "phasar/Utils/TypeTraits.h"

namespace psr {

template <typename AnalysisDomainTy, typename = void>
struct DefaultAnalysisPrinterSelector
    : type_identity<DefaultAnalysisPrinter<AnalysisDomainTy>> {};

} // namespace psr

#endif // PHASAR_DATAFLOW_IFDSIDE_DEFAULTANALYSISPRINTERSELECTOR_H
