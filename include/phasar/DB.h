/******************************************************************************
 * Copyright (c) 2023 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_DB_H
#define PHASAR_DB_H

#include "phasar/Config/phasar-config.h"
#include "phasar/DB/ProjectIRDBBase.h"

#ifdef PHASAR_HAS_SQLITE
#include "phasar/DB/Hexastore.h"
#include "phasar/DB/Queries.h"
#endif

#endif // PHASAR_DB_H
