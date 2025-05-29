module;

#include "phasar/DB/Hexastore.h"
#include "phasar/DB/ProjectIRDBBase.h"
#include "phasar/DB/Queries.h"

export module phasar.db;

export namespace psr {
using psr::IRDBGetFunctionDef;
using psr::ProjectIRDBBase;
using psr::ProjectIRDBTraits;

#ifdef PHASAR_HAS_SQLITE
using psr::Hexastore;
using psr::HSResult;
using psr::INIT;
using psr::OPSInsert;
using psr::OSPInsert;
using psr::POSInsert;
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
#endif
} // namespace psr
