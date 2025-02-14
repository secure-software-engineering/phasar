/******************************************************************************
 * Copyright (c) 2024 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_UTILS_MACROS_H
#define PHASAR_UTILS_MACROS_H

#if __cplusplus < 202002L
#define PSR_CONCEPT static constexpr bool
#else
#define PSR_CONCEPT concept
#endif

#endif // PHASAR_UTILS_MACROS_H
