#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/TypeStateDescriptions/OpenSSLEVPMDCTXDescription.h"

#include "llvm/ADT/StringSwitch.h"
#include "llvm/Support/ErrorHandling.h"
#include <llvm/ADT/SmallVector.h>

using namespace std;

namespace psr {

const std::array<
    int, enum2int(OpenSSLEVPMDCTXDescription::OpenSSLEVPMDCTXToken::STAR)>
    OpenSSLEVPMDCTXDescription::OpenSSLEVPMDCTXFuncs = {-1, 0, 0, 0,
                                                        0,  0, 0, 0};

// delta[Token][State] = next State
// Tokens: NEW = 0, INIT, UPDATE, FINAL, FREE, STAR
// States: BOT = 0, ALLOCATED = 1, INITIALIZED = 2, SIGN_INITIALIZED = 3,
// FINALIZED = 4, FREED = 5, ERROR = 6, UNINIT = 7
const OpenSSLEVPMDCTXDescription::OpenSSLEVPMDCTXState
    OpenSSLEVPMDCTXDescription::Delta[enum2int(OpenSSLEVPMDCTXToken::STAR) +
                                      1][enum2int(
                                             OpenSSLEVPMDCTXState::UNINIT) +
                                         1] = {
        // NEW
        {ALLOCATED, ALLOCATED, ALLOCATED, ALLOCATED, ALLOCATED, ALLOCATED,
         ERROR, ALLOCATED},
        // INIT
        {BOT, INITIALIZED, INITIALIZED, INITIALIZED, INITIALIZED, ERROR, ERROR,
         ERROR},
        // UPDATE
        {BOT, ERROR, INITIALIZED, SIGN_INITIALIZED, ERROR, ERROR, ERROR, ERROR},
        // FINAL
        {BOT, ERROR, FINALIZED, ERROR, ERROR, ERROR, ERROR, ERROR},
        // SIGN_INIT
        {BOT, SIGN_INITIALIZED, SIGN_INITIALIZED, SIGN_INITIALIZED,
         SIGN_INITIALIZED, ERROR, ERROR, ERROR},
        // SIGN_UPDATE
        {BOT, ERROR, ERROR, SIGN_INITIALIZED, ERROR, ERROR, ERROR, ERROR},
        // SIGN_FINAL
        {BOT, ERROR, ERROR, FINALIZED, ERROR, ERROR, ERROR, ERROR},
        // FREE
        {ERROR, FREED, FREED, FREED, FREED, ERROR, ERROR, ERROR},
        // STAR
        {BOT, ALLOCATED, INITIALIZED, SIGN_INITIALIZED, FINALIZED, ERROR, ERROR,
         ERROR}

};

OpenSSLEVPMDCTXDescription::OpenSSLEVPMDCTXToken
OpenSSLEVPMDCTXDescription::funcNameToToken(llvm::StringRef F) const {
  /*return llvm::StringSwitch<OpenSSLEVPMDCTXToken>(F)
      .Case("EVP_MD_CTX_new", OpenSSLEVPMDCTXToken::EVP_MD_CTX_NEW)
      .Case("EVP_DigestInit", OpenSSLEVPMDCTXToken::EVP_DIGEST_INIT)
      .Case("EVP_DigestInit_ex", OpenSSLEVPMDCTXToken::EVP_DIGEST_INIT)
      .Case("EVP_DigestUpdate", OpenSSLEVPMDCTXToken::EVP_DIGEST_UPDATE)
      .Case("EVP_DigestFinal", OpenSSLEVPMDCTXToken::EVP_DIGEST_FINAL)
      .Case("EVP_DigestFinal_ex", OpenSSLEVPMDCTXToken::EVP_DIGEST_FINAL)
      .Case("EVP_DigestSignInit", OpenSSLEVPMDCTXToken::EVP_DIGEST_SIGN_INIT)
      .Case("EVP_DigestSignInit_ex", OpenSSLEVPMDCTXToken::EVP_DIGEST_SIGN_INIT)
      .Case("EVP_DigestSignUpdate",
            OpenSSLEVPMDCTXToken::EVP_DIGEST_SIGN_UPDATE)
      .Case("EVP_DigestSignFinal", OpenSSLEVPMDCTXToken::EVP_DIGEST_SIGN_FINAL)
      .Case("EVP_DigestSignFinal_ex",
            OpenSSLEVPMDCTXToken::EVP_DIGEST_SIGN_FINAL)
      .Case("EVP_MD_CTX_free", OpenSSLEVPMDCTXToken::EVP_MD_CTX_FREE)
      .Default(OpenSSLEVPMDCTXToken::STAR);*/
  if (auto it = name2tok.find(F); it != name2tok.end()) {
    return it->second;
  }
  return OpenSSLEVPMDCTXToken::STAR;
}

OpenSSLEVPMDCTXDescription::OpenSSLEVPMDCTXDescription(
    const stringstringmap_t *staticRenaming)
    : TypeStateDescription(), staticRenaming(staticRenaming),
      name2tok(
          {{"EVP_MD_CTX_new", OpenSSLEVPMDCTXToken::EVP_MD_CTX_NEW},
           {"EVP_DigestInit", OpenSSLEVPMDCTXToken::EVP_DIGEST_INIT},
           {"EVP_DigestInit_ex", OpenSSLEVPMDCTXToken::EVP_DIGEST_INIT},
           {"EVP_DigestUpdate", OpenSSLEVPMDCTXToken::EVP_DIGEST_UPDATE},
           {"EVP_DigestFinal", OpenSSLEVPMDCTXToken::EVP_DIGEST_FINAL},
           {"EVP_DigestFinal_ex", OpenSSLEVPMDCTXToken::EVP_DIGEST_FINAL},
           {"EVP_DigestSignInit", OpenSSLEVPMDCTXToken::EVP_DIGEST_SIGN_INIT},
           {"EVP_DigestSignInit_ex",
            OpenSSLEVPMDCTXToken::EVP_DIGEST_SIGN_INIT},
           {"EVP_DigestSignUpdate",
            OpenSSLEVPMDCTXToken::EVP_DIGEST_SIGN_UPDATE},
           {"EVP_DigestSignFinal", OpenSSLEVPMDCTXToken::EVP_DIGEST_SIGN_FINAL},
           {"EVP_DigestSignFinal_ex",
            OpenSSLEVPMDCTXToken::EVP_DIGEST_SIGN_FINAL},
           {"EVP_MD_CTX_free", OpenSSLEVPMDCTXToken::EVP_MD_CTX_FREE}}) {
  if (staticRenaming) {
    llvm::SmallVector<std::pair<llvm::StringRef, OpenSSLEVPMDCTXToken>, 12>
        toinsert;
    for (auto &entry : name2tok) {
      auto it = staticRenaming->find(entry.getKey());
      if (it != staticRenaming->end()) {
        toinsert.emplace_back(it->second, entry.getValue());
      }
    }
    for (auto &[key, value] : toinsert) {
      name2tok[key] = value;
    }
  }
}

bool OpenSSLEVPMDCTXDescription::isFactoryFunction(const std::string &F) const {
  auto tok = funcNameToToken(F);
  return tok != OpenSSLEVPMDCTXToken::STAR &&
         OpenSSLEVPMDCTXFuncs[enum2int(tok)] == -1;
}

bool OpenSSLEVPMDCTXDescription::isConsumingFunction(
    const std::string &F) const {
  auto tok = funcNameToToken(F);
  return tok != OpenSSLEVPMDCTXToken::STAR &&
         OpenSSLEVPMDCTXFuncs[enum2int(tok)] >= 0;
}
bool OpenSSLEVPMDCTXDescription::isAPIFunction(const std::string &F) const {
  return funcNameToToken(F) != OpenSSLEVPMDCTXToken::STAR;
}
TypeStateDescription::State
OpenSSLEVPMDCTXDescription::getNextState(std::string Tok,
                                         TypeStateDescription::State S) const {
  auto tok = funcNameToToken(Tok);

  return Delta[enum2int(tok)][S];
}

std::string OpenSSLEVPMDCTXDescription::getTypeNameOfInterest() const {
  if (staticRenaming) {
    if (auto it = staticRenaming->find("evp_md_ctx_st");
        it != staticRenaming->end()) {
      return (llvm::StringLiteral("struct.") + it->second).str();
    }
  }
  return "struct.evp_md_ctx_st";
}

std::set<int>
OpenSSLEVPMDCTXDescription::getConsumerParamIdx(const std::string &F) const {
  auto tok = funcNameToToken(F);
  if (tok == OpenSSLEVPMDCTXToken::STAR)
    return {};

  auto idx = OpenSSLEVPMDCTXFuncs[enum2int(tok)];
  if (idx >= 0)
    return {idx};
  else
    return {};
}
std::set<int>
OpenSSLEVPMDCTXDescription::getFactoryParamIdx(const std::string &F) const {
  auto tok = funcNameToToken(F);
  if (tok == OpenSSLEVPMDCTXToken::STAR)
    return {};

  auto idx = OpenSSLEVPMDCTXFuncs[enum2int(tok)];
  if (idx == -1)
    return {-1};
  else
    return {};
}

std::string
OpenSSLEVPMDCTXDescription::stateToString(TypeStateDescription::State S) const {
  switch (S) {
  case TOP:
    return "TOP";
  case BOT:
    return "BOT";
  case ALLOCATED:
    return "ALLOCATED";
  case INITIALIZED:
    return "INITIALIZED";
  case FINALIZED:
    return "FINALIZED";
  case SIGN_INITIALIZED:
    return "SIGN_INITIALIZED";
  case FREED:
    return "FREED";
  case ERROR:
    return "ERROR";
  case UNINIT:
    return "UNINIT";
  default:
    return "<NONE>";
  }
}

TypeStateDescription::State OpenSSLEVPMDCTXDescription::bottom() const {
  return BOT;
}

TypeStateDescription::State OpenSSLEVPMDCTXDescription::top() const {
  return TOP;
}

TypeStateDescription::State OpenSSLEVPMDCTXDescription::uninit() const {
  return UNINIT;
}

TypeStateDescription::State OpenSSLEVPMDCTXDescription::start() const {
  return ALLOCATED;
}

TypeStateDescription::State OpenSSLEVPMDCTXDescription::error() const {
  return ERROR;
}

} // namespace psr