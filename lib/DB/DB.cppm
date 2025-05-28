module;

#include "phasar/DB/Hexastore.h"
#include "phasar/DB/ProjectIRDBBase.h"
#include "phasar/DB/Queries.h"

export module phasar.db;

export namespace psr {
#ifndef PHASAR_HAS_SQLITE
#error                                                                         \
    "Hexastore requires SQLite3. Please install libsqlite3-dev and reconfigure PhASAR."
using psr::Hexastore;
using psr::HSResult;
#endif
using psr::INIT;
using psr::IRDBGetFunctionDef;
using psr::OPSInsert;
using psr::OSPInsert;
using psr::POSInsert;
using psr::ProjectIRDBBase;
using psr::ProjectIRDBTraits;
using psr::PSOInsert;
using psr::SearchSPO;
using psr::SearchSPX;
using psr::SearchSXO;
using psr::SearchSXX;
using psr::SearchXPO;
using psr::SearchXPX;
using psr::SearchXXO;
using psr::SearchXXX;
using psr::SOPInsert;
using psr::SPOInsert;
} // namespace psr
