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

#define PSR_FWD(...) ::std::forward<decltype(__VA_ARGS__)>(__VA_ARGS__)

#endif // PHASAR_UTILS_MACROS_H
