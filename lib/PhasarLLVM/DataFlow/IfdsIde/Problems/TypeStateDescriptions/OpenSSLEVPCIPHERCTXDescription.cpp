#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/TypeStateDescriptions/OpenSSLEVPCIPHERCTXDescription.h"

#include "phasar/Utils/EnumFlags.h"

#include "llvm/ADT/StringSwitch.h"
namespace psr {

const std::array<
    int,
    enum2int(OpenSSLEVPCIPHERCTXDescription::OpenSSLEVPCIPHERCTXToken::STAR)>
    OpenSSLEVPCIPHERCTXDescription::OpenSSLEVPCIPHERCTXFuncs = {
        -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

#if __cplusplus >= 202002L
using enum OpenSSLEVPCIPHERCTXState;
#else
static constexpr auto TOP = OpenSSLEVPCIPHERCTXState::TOP;
static constexpr auto BOT = OpenSSLEVPCIPHERCTXState::BOT;
static constexpr auto ALLOCATED = OpenSSLEVPCIPHERCTXState::ALLOCATED;
static constexpr auto INITIALIZED_CIPHER =
    OpenSSLEVPCIPHERCTXState::INITIALIZED_CIPHER;
static constexpr auto INITIALIZED_ENCRYPT =
    OpenSSLEVPCIPHERCTXState::INITIALIZED_ENCRYPT;
static constexpr auto INITIALIZED_DECRYPT =
    OpenSSLEVPCIPHERCTXState::INITIALIZED_DECRYPT;
static constexpr auto FINALIZED = OpenSSLEVPCIPHERCTXState::FINALIZED;
static constexpr auto FREED = OpenSSLEVPCIPHERCTXState::FREED;
static constexpr auto ERROR = OpenSSLEVPCIPHERCTXState::ERROR;
static constexpr auto UNINIT = OpenSSLEVPCIPHERCTXState::UNINIT;
#endif

// delta[Token][State] = next State
//
// Tokens: EVP_CIPHER_CTX_NEW, EVP_CIPHER_INIT, EVP_CIPHER_UPDATE,
// EVP_CIPHER_FINAL, EVP_ENCRYPT_INIT, EVP_ENCRYPT_UPDATE, EVP_ENCRYPT_FINAL,
// EVP_DECRYPT_INIT, EVP_DECRYPT_UPDATE, EVP_DECRYPT_FINAL, EVP_CIPHER_CTX_FREE,
// STAR
//
// States: BOT, ALLOCATED,  INITIALIZED_CIPHER, INITIALIZED_ENCRYPT,
// INITIALIZED_DECRYPT, FINALIZED, FREED, ERROR, UNINIT
//
// TODO: Do we allow mixing Cipher/ENcrypt/Decrypt functions?
const OpenSSLEVPCIPHERCTXState OpenSSLEVPCIPHERCTXDescription::Delta
    [enum2int(OpenSSLEVPCIPHERCTXToken::STAR) + 1]
    [enum2int(OpenSSLEVPCIPHERCTXState::UNINIT) + 1] = {
        // EVP_CIPHER_CTX_NEW
        {ALLOCATED, ALLOCATED, ALLOCATED, ALLOCATED, ALLOCATED, ALLOCATED,
         ALLOCATED, ERROR, ALLOCATED},
        // EVP_CIPHER_INIT,
        {BOT, INITIALIZED_CIPHER, INITIALIZED_CIPHER, INITIALIZED_CIPHER,
         INITIALIZED_CIPHER, INITIALIZED_CIPHER, ERROR, ERROR, ERROR},
        // EVP_CIPHER_UPDATE,
        {BOT, ERROR, INITIALIZED_CIPHER, ERROR, ERROR, ERROR, ERROR, ERROR,
         ERROR},
        // EVP_CIPHER_FINAL,
        {BOT, ERROR, FINALIZED, ERROR, ERROR, ERROR, ERROR, ERROR, ERROR},
        // EVP_ENCRYPT_INIT,
        {BOT, INITIALIZED_ENCRYPT, INITIALIZED_ENCRYPT, INITIALIZED_ENCRYPT,
         INITIALIZED_ENCRYPT, ERROR, ERROR, ERROR, ERROR},
        // EVP_ENCRYPT_UPDATE,
        {BOT, ERROR, ERROR, INITIALIZED_ENCRYPT, ERROR, ERROR, ERROR, ERROR,
         ERROR},
        // EVP_ENCRYPT_FINAL,
        {BOT, ERROR, ERROR, FINALIZED, ERROR, ERROR, ERROR, ERROR, ERROR},
        // EVP_DECRYPT_INIT,
        {BOT, INITIALIZED_DECRYPT, INITIALIZED_DECRYPT, INITIALIZED_DECRYPT,
         INITIALIZED_DECRYPT, INITIALIZED_DECRYPT, ERROR, ERROR, ERROR},
        // EVP_DECRYPT_UPDATE,
        {BOT, ERROR, ERROR, ERROR, INITIALIZED_DECRYPT, ERROR, ERROR, ERROR,
         ERROR},
        // EVP_DECRYPT_FINAL,
        {BOT, ERROR, ERROR, ERROR, FINALIZED, ERROR, ERROR, ERROR, ERROR},
        // EVP_CIPHER_CTX_FREE,
        {ERROR, FREED, FREED, FREED, FREED, FREED, ERROR, ERROR, ERROR},
        // STAR
        {BOT, ALLOCATED, INITIALIZED_CIPHER, INITIALIZED_ENCRYPT,
         INITIALIZED_DECRYPT, FINALIZED, ERROR, ERROR, ERROR},
};

auto OpenSSLEVPCIPHERCTXDescription::funcNameToToken(llvm::StringRef F) const
    -> OpenSSLEVPCIPHERCTXToken {

  /*return llvm::StringSwitch<OpenSSLEVPCIPHERCTXToken>(F)
      .Case("EVP_CIPHER_CTX_new", OpenSSLEVPCIPHERCTXToken::EVP_CIPHER_CTX_NEW)
      .Cases("EVP_CipherInit", "EVP_CipherInit_ex",
             OpenSSLEVPCIPHERCTXToken::EVP_CIPHER_INIT)
      .Case("EVP_CipherUpdate", OpenSSLEVPCIPHERCTXToken::EVP_CIPHER_UPDATE)
      .Cases("EVP_CipherFinal", "EVP_CipherFinal_ex",
             OpenSSLEVPCIPHERCTXToken::EVP_CIPHER_FINAL)
      .Cases("EVP_EncryptInit", "EVP_EncryptInit_ex",
             OpenSSLEVPCIPHERCTXToken::EVP_ENCRYPT_INIT)
      .Case("EVP_EncryptUpdate", OpenSSLEVPCIPHERCTXToken::EVP_ENCRYPT_UPDATE)
      .Cases("EVP_EncryptFinal", "EVP_EncryptFinal_ex",
             OpenSSLEVPCIPHERCTXToken::EVP_ENCRYPT_FINAL)
      .Cases("EVP_DecryptInit", "EVP_DecryptInit_ex",
             OpenSSLEVPCIPHERCTXToken::EVP_DECRYPT_INIT)
      .Case("EVP_DecryptUpdate", OpenSSLEVPCIPHERCTXToken::EVP_DECRYPT_UPDATE)
      .Cases("EVP_DecryptFinal", "EVP_DecryptFinal_ex",
             OpenSSLEVPCIPHERCTXToken::EVP_DECRYPT_FINAL)
      .Default(OpenSSLEVPCIPHERCTXToken::STAR);*/
  if (auto it = name2tok.find(F); it != name2tok.end()) {
    return it->second;
  }
  return OpenSSLEVPCIPHERCTXToken::STAR;
}

OpenSSLEVPCIPHERCTXDescription::OpenSSLEVPCIPHERCTXDescription(
    const stringstringmap_t *staticRenaming, llvm::StringRef typeNameOfInterest)
    : TypeStateDescription(),
      name2tok(
          {{"EVP_CIPHER_CTX_new", OpenSSLEVPCIPHERCTXToken::EVP_CIPHER_CTX_NEW},
           {"EVP_CipherInit_ex", OpenSSLEVPCIPHERCTXToken::EVP_CIPHER_INIT},
           {"EVP_CipherInit", OpenSSLEVPCIPHERCTXToken::EVP_CIPHER_INIT},
           {"EVP_CipherUpdate", OpenSSLEVPCIPHERCTXToken::EVP_CIPHER_UPDATE},
           {"EVP_CipherFinal", OpenSSLEVPCIPHERCTXToken::EVP_CIPHER_FINAL},
           {"EVP_CipherFinal_ex", OpenSSLEVPCIPHERCTXToken::EVP_CIPHER_FINAL},
           {"EVP_EncryptInit", OpenSSLEVPCIPHERCTXToken::EVP_ENCRYPT_INIT},
           {"EVP_EncryptInit_ex", OpenSSLEVPCIPHERCTXToken::EVP_ENCRYPT_INIT},
           {"EVP_EncryptUpdate", OpenSSLEVPCIPHERCTXToken::EVP_ENCRYPT_UPDATE},
           {"EVP_EncryptFinal", OpenSSLEVPCIPHERCTXToken::EVP_ENCRYPT_FINAL},
           {"EVP_EncryptFinal_ex", OpenSSLEVPCIPHERCTXToken::EVP_ENCRYPT_FINAL},
           {"EVP_DecryptInit", OpenSSLEVPCIPHERCTXToken::EVP_DECRYPT_INIT},
           {"EVP_DecryptInit_ex", OpenSSLEVPCIPHERCTXToken::EVP_DECRYPT_INIT},
           {"EVP_DecryptUpdate", OpenSSLEVPCIPHERCTXToken::EVP_DECRYPT_UPDATE},
           {"EVP_DecryptFinal", OpenSSLEVPCIPHERCTXToken::EVP_DECRYPT_FINAL},
           {"EVP_DecryptFinal_ex", OpenSSLEVPCIPHERCTXToken::EVP_DECRYPT_FINAL},
           {"EVP_CIPHER_CTX_FREE",
            OpenSSLEVPCIPHERCTXToken::EVP_CIPHER_CTX_FREE}}),
      typeNameOfInterest(typeNameOfInterest.str()) {
  if (staticRenaming) {
    llvm::SmallVector<std::pair<llvm::StringRef, OpenSSLEVPCIPHERCTXToken>, 12>
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

bool OpenSSLEVPCIPHERCTXDescription::isFactoryFunction(
    llvm::StringRef F) const {
  auto tok = funcNameToToken(F);
  return tok != OpenSSLEVPCIPHERCTXToken::STAR &&
         OpenSSLEVPCIPHERCTXFuncs[enum2int(tok)] == -1;
}

bool OpenSSLEVPCIPHERCTXDescription::isConsumingFunction(
    llvm::StringRef F) const {
  auto tok = funcNameToToken(F);
  return tok != OpenSSLEVPCIPHERCTXToken::STAR &&
         OpenSSLEVPCIPHERCTXFuncs[enum2int(tok)] >= 0;
}
bool OpenSSLEVPCIPHERCTXDescription::isAPIFunction(llvm::StringRef F) const {
  return funcNameToToken(F) != OpenSSLEVPCIPHERCTXToken::STAR;
}
auto OpenSSLEVPCIPHERCTXDescription::getNextState(
    llvm::StringRef Tok, TypeStateDescription::State S) const -> State {
  auto tok = funcNameToToken(Tok);

  return Delta[enum2int(tok)][enum2int(S)];
}

std::string OpenSSLEVPCIPHERCTXDescription::getTypeNameOfInterest() const {
  /*if (staticRenaming) {
    if (auto it = staticRenaming->find("evp_cipher_ctx_st");
        it != staticRenaming->end()) {
      return (llvm::StringLiteral("struct.") + it->second).str();
    }
  }
  return "struct.evp_cipher_ctx_st";*/
  return typeNameOfInterest;
}

std::set<int>
OpenSSLEVPCIPHERCTXDescription::getConsumerParamIdx(llvm::StringRef F) const {
  auto tok = funcNameToToken(F);
  if (tok == OpenSSLEVPCIPHERCTXToken::STAR)
    return {};

  auto idx = OpenSSLEVPCIPHERCTXFuncs[enum2int(tok)];
  if (idx >= 0)
    return {idx};
  else
    return {};
}
std::set<int>
OpenSSLEVPCIPHERCTXDescription::getFactoryParamIdx(llvm::StringRef F) const {
  auto tok = funcNameToToken(F);
  if (tok == OpenSSLEVPCIPHERCTXToken::STAR)
    return {};

  auto idx = OpenSSLEVPCIPHERCTXFuncs[enum2int(tok)];
  if (idx == -1)
    return {-1};
  else
    return {};
}

static llvm::StringRef stateToUnownedString(OpenSSLEVPCIPHERCTXState S) {
  switch (S) {
  case TOP:
    return "TOP";
  case BOT:
    return "BOT";
  case ALLOCATED:
    return "ALLOCATED";
  case INITIALIZED_CIPHER:
    return "INITIALIZED_CIPHER";
  case INITIALIZED_ENCRYPT:
    return "INITIALIZED_ENCRYPT";
  case INITIALIZED_DECRYPT:
    return "INITIALIZED_DECRYPT";
  case FINALIZED:
    return "FINALIZED";
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

std::string OpenSSLEVPCIPHERCTXDescription::stateToString(
    TypeStateDescription::State S) const {
  return stateToUnownedString(S).str();
}

llvm::StringRef OpenSSLEVPCIPHERCTXDescription::tokenToString(int tok) const {
  static std::array<std::string, enum2int(OpenSSLEVPCIPHERCTXToken::STAR)>
      tokNames = {
          "EVP_CIPHER_CTX_NEW", "EVP_CIPHER_INIT",    "EVP_CIPHER_UPDATE",
          "EVP_CIPHER_FINAL",   "EVP_ENCRYPT_INIT",   "EVP_ENCRYPT_UPDATE",
          "EVP_ENCRYPT_FINAL",  "EVP_DECRYPT_INIT",   "EVP_DECRYPT_UPDATE",
          "EVP_DECRYPT_FINAL",  "EVP_CIPHER_CTX_FREE"};
  if (unsigned(tok) < tokNames.size())
    return tokNames[tok];

  return "<STAR>";
}

llvm::StringRef
OpenSSLEVPCIPHERCTXDescription::demangleToken(llvm::StringRef Tok) const {
  auto IntTok = funcNameToToken(Tok);
  if (IntTok == OpenSSLEVPCIPHERCTXToken::STAR)
    return Tok;
  return tokenToString(enum2int(IntTok));
}

OpenSSLEVPCIPHERCTXState OpenSSLEVPCIPHERCTXDescription::bottom() const {
  return BOT;
}

OpenSSLEVPCIPHERCTXState OpenSSLEVPCIPHERCTXDescription::top() const {
  return TOP;
}

OpenSSLEVPCIPHERCTXState OpenSSLEVPCIPHERCTXDescription::uninit() const {
  return UNINIT;
}

OpenSSLEVPCIPHERCTXState OpenSSLEVPCIPHERCTXDescription::start() const {
  return ALLOCATED;
}

OpenSSLEVPCIPHERCTXState OpenSSLEVPCIPHERCTXDescription::error() const {
  return ERROR;
}
} // namespace psr