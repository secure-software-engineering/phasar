#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/TypeStateDescriptions/OpenSSLEVPMDCTXDescription.h"

#include "phasar/Utils/EnumFlags.h"

#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/Support/ErrorHandling.h"

using namespace std;

namespace psr {

const std::array<
    int, enum2int(OpenSSLEVPMDCTXDescription::OpenSSLEVPMDCTXToken::STAR)>
    OpenSSLEVPMDCTXDescription::OpenSSLEVPMDCTXFuncs = {-1, 0, 0, 0,
                                                        0,  0, 0, 0};

#if __cplusplus >= 202002L
using enum OpenSSLEVPMDCTXState;
#else
static constexpr auto TOP = OpenSSLEVPMDCTXState::TOP;
static constexpr auto BOT = OpenSSLEVPMDCTXState::BOT;
static constexpr auto ALLOCATED = OpenSSLEVPMDCTXState::ALLOCATED;
static constexpr auto INITIALIZED = OpenSSLEVPMDCTXState::INITIALIZED;
static constexpr auto SIGN_INITIALIZED = OpenSSLEVPMDCTXState::SIGN_INITIALIZED;
static constexpr auto FINALIZED = OpenSSLEVPMDCTXState::FINALIZED;
static constexpr auto FREED = OpenSSLEVPMDCTXState::FREED;
static constexpr auto ERROR = OpenSSLEVPMDCTXState::ERROR;
static constexpr auto UNINIT = OpenSSLEVPMDCTXState::UNINIT;
#endif

// delta[Token][State] = next State
// Tokens: NEW = 0, INIT, UPDATE, FINAL, FREE, STAR
// States: BOT = 0, ALLOCATED = 1, INITIALIZED = 2, SIGN_INITIALIZED = 3,
// FINALIZED = 4, FREED = 5, ERROR = 6, UNINIT = 7
const OpenSSLEVPMDCTXState OpenSSLEVPMDCTXDescription::Delta
    [enum2int(OpenSSLEVPMDCTXToken::STAR) + 1]
    [enum2int(OpenSSLEVPMDCTXState::UNINIT) + 1] = {
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
    const stringstringmap_t *staticRenaming, llvm::StringRef typeNameOfInterest)
    : TypeStateDescription(),
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
           {"EVP_MD_CTX_free", OpenSSLEVPMDCTXToken::EVP_MD_CTX_FREE}}),
      typeNameOfInterest(typeNameOfInterest.str()) {
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

bool OpenSSLEVPMDCTXDescription::isFactoryFunction(llvm::StringRef F) const {
  auto tok = funcNameToToken(F);
  return tok != OpenSSLEVPMDCTXToken::STAR &&
         OpenSSLEVPMDCTXFuncs[enum2int(tok)] == -1;
}

bool OpenSSLEVPMDCTXDescription::isConsumingFunction(llvm::StringRef F) const {
  auto tok = funcNameToToken(F);
  return tok != OpenSSLEVPMDCTXToken::STAR &&
         OpenSSLEVPMDCTXFuncs[enum2int(tok)] >= 0;
}
bool OpenSSLEVPMDCTXDescription::isAPIFunction(llvm::StringRef F) const {
  return funcNameToToken(F) != OpenSSLEVPMDCTXToken::STAR;
}
OpenSSLEVPMDCTXState
OpenSSLEVPMDCTXDescription::getNextState(llvm::StringRef Tok,
                                         TypeStateDescription::State S) const {
  auto tok = funcNameToToken(Tok);

  // std::cerr << "nextState(" << Tok << ", " << stateToString(S)
  //           << ") = " << stateToString(Delta[enum2int(tok)][S]) << "\n";

  return Delta[enum2int(tok)][enum2int(S)];
}

std::string OpenSSLEVPMDCTXDescription::getTypeNameOfInterest() const {
  /*if (staticRenaming) {
    if (auto it = staticRenaming->find("evp_md_ctx_st");
        it != staticRenaming->end()) {
      return (llvm::StringLiteral("struct.") + it->second).str();
    }
  }
  return "struct.evp_md_ctx_st";*/
  return typeNameOfInterest;
}

std::set<int>
OpenSSLEVPMDCTXDescription::getConsumerParamIdx(llvm::StringRef F) const {
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
OpenSSLEVPMDCTXDescription::getFactoryParamIdx(llvm::StringRef F) const {
  auto tok = funcNameToToken(F);
  if (tok == OpenSSLEVPMDCTXToken::STAR)
    return {};

  auto idx = OpenSSLEVPMDCTXFuncs[enum2int(tok)];
  if (idx == -1)
    return {-1};
  else
    return {};
}

static llvm::StringRef stateToUnownedString(OpenSSLEVPMDCTXState S) {
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

std::string
OpenSSLEVPMDCTXDescription::stateToString(TypeStateDescription::State S) const {
  return stateToUnownedString(S).str();
}
llvm::StringRef OpenSSLEVPMDCTXDescription::tokenToString(int tok) const {
  static std::array<std::string, enum2int(OpenSSLEVPMDCTXToken::STAR)>
      tokNames = {"EVP_MD_CTX_NEW",        "EVP_DIGEST_INIT",
                  "EVP_DIGEST_UPDATE",     "EVP_DIGEST_FINAL",
                  "EVP_DIGEST_SIGN_INIT",  "EVP_DIGEST_SIGN_UPDATE",
                  "EVP_DIGEST_SIGN_FINAL", "EVP_MD_CTX_FREE"};
  if (unsigned(tok) < tokNames.size())
    return tokNames[tok];

  return "<STAR>";
}

llvm::StringRef
OpenSSLEVPMDCTXDescription::demangleToken(llvm::StringRef Tok) const {
  auto IntTok = funcNameToToken(Tok);
  if (IntTok == OpenSSLEVPMDCTXToken::STAR)
    return Tok;
  return tokenToString(enum2int(IntTok));
}

OpenSSLEVPMDCTXState OpenSSLEVPMDCTXDescription::bottom() const { return BOT; }

OpenSSLEVPMDCTXState OpenSSLEVPMDCTXDescription::top() const { return TOP; }

OpenSSLEVPMDCTXState OpenSSLEVPMDCTXDescription::uninit() const {
  return UNINIT;
}

OpenSSLEVPMDCTXState OpenSSLEVPMDCTXDescription::start() const {
  return ALLOCATED;
}

OpenSSLEVPMDCTXState OpenSSLEVPMDCTXDescription::error() const { return ERROR; }

} // namespace psr