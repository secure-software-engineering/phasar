#include "PTAUtils.h"

#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Pointer/AliasResult.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/SHA256.h"
#include "llvm/Support/raw_ostream.h"

#include "AliasQueryType.h"

#include <cstdint>

using namespace psr;

bool ptaben::isSoundResult(const QueryResult &QR) noexcept {
  auto Res = QR.Result;
  auto ExpectedRes = getExpectedAliasResult(QR.QueryType);

  return Res == ExpectedRes || Res == AliasResult::MayAlias;
}

llvm::raw_ostream &ptaben::operator<<(llvm::raw_ostream &OS,
                                      const QueryResult &Result) {
  OS << "Query at " << psr::llvmIRToString(Result.Inst) << " expected "
     << to_string(Result.QueryType) << ", but got " << to_string(Result.Result);
  return OS;
}

llvm::raw_ostream &ptaben::operator<<(llvm::raw_ostream &OS,
                                      const QueryLocation &Result) {
  return OS << "Query at " << psr::llvmIRToString(Result.Inst);
}

static ptaben::QueryId getQueryId(llvm::StringRef FileName, uint32_t SeqNo) {
  llvm::SHA256 Hasher;
  Hasher.update(FileName);

  {
    std::array<uint8_t, 4> Buf{};
    llvm::support::endian::write32(Buf.data(), SeqNo,
                                   llvm::support::endianness::little);
    Hasher.update(Buf);
  }

  auto Hash = Hasher.final();

  auto QId = llvm::support::endian::read64(Hash.data(),
                                           llvm::support::endianness::little);

  return ptaben::QueryId(QId);
}

void ptaben::findAllQueryLocations(
    const llvm::Module &Mod, llvm::SmallVectorImpl<QueryLocation> &Locs,
    llvm::SmallVectorImpl<QuerySrcCodeLocation> *SrcLocs) {
  const auto &FilePath = Mod.getSourceFileName();
  auto FileName = llvm::sys::path::filename(FilePath);
  uint32_t Ctr = 0;

  for (const auto &Fun : Mod.functions()) {
    for (const auto &Inst : llvm::instructions(Fun)) {
      if (const auto *Call = llvm::dyn_cast<llvm::CallBase>(&Inst);
          Call && Call->getCalledFunction()) {
        if (auto QueryType =
                parseAliasQueryType(Call->getCalledFunction()->getName())) {
          auto QId = getQueryId(FileName, Ctr++);
          Locs.push_back(QueryLocation{QId, Call, *QueryType});

          if (SrcLocs) {
            SrcLocs->push_back(QuerySrcCodeLocation{FilePath, Ctr});
          }
        }
      }
    }
  }
}

bool ptaben::verifyAnalysisResult(AliasResult Res,
                                  const QueryLocation &QueryLoc) {
  switch (QueryLoc.QueryType) {
    using enum ptaben::AliasQueryType;
  case EXPECTEDFAILMAYALIAS:
    if (Res != psr::AliasResult::MayAlias) {
      llvm::outs() << "[INFO]: Failed EXPECTEDFAIL_MAYALIAS at" << QueryLoc
                   << '\n';
    }
    return true;
  case PARTIALALIAS:
    // We don't support partial alias. Use may alias instead.
    [[fallthrough]];
  case MAYALIAS:
    if (Res == psr::AliasResult::MayAlias) {
      return true;
    }
    break;

  case EXPECTEDFAILMUSTALIAS:
    if (Res != psr::AliasResult::MustAlias) {
      llvm::outs() << "[INFO]: Failed EXPECTEDFAIL_MUSTALIAS at" << QueryLoc
                   << '\n';
    }
    return true;
  case MUSTALIAS:
    if (Res == psr::AliasResult::MustAlias) {
      return true;
    }
    break;

  case EXPECTEDFAILNOALIAS:
    if (Res != psr::AliasResult::NoAlias) {
      llvm::outs() << "[INFO]: Failed EXPECTEDFAIL_NOALIAS at" << QueryLoc
                   << '\n';
    }
    return true;
  case NOALIAS:
    if (Res == psr::AliasResult::NoAlias) {
      return true;
    }
    break;
  }

  return false;
}
