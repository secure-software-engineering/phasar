#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"

#include "phasar/Config/Configuration.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/Macros.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/IR/AssemblyAnnotationWriter.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/MemoryBufferRef.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/WithColor.h"

#include <charconv>
#include <memory>
#include <system_error>

namespace psr {

[[deprecated]]
static void setOpaquePointersForCtx(llvm::LLVMContext &Ctx, bool Enable) {
#if LLVM_VERSION_MAJOR >= 15 && LLVM_VERSION_MAJOR < 17
  if (!Enable) {
    Ctx.setOpaquePointers(false);
  }
#elif LLVM_VERSION_MAJOR < 15
  if (Enable) {
    Ctx.enableOpaquePointers();
  }
#else // LLVM_VERSION_MAJOR >= 17
#error                                                                         \
    "Non-opaque pointers are not supported anymore. Refactor PhASAR to remove typed pointer support."
#endif
}

namespace {
enum class IRDBParsingError {
  CouldNotParse = 1,
  CouldNotVerify = 2,
};

class IRDBParsingErrorCategory : public std::error_category {
  [[nodiscard]] const char *name() const noexcept override {
    return "IRDBParsingError";
  }

  [[nodiscard]] std::string message(int Value) const override {
    switch (IRDBParsingError(Value)) {
    case IRDBParsingError::CouldNotParse:
      return "Could not parse LLVM IR";
    case IRDBParsingError::CouldNotVerify:
      return "Parsed LLVM IR could not be verified";
    default:
      return "Unknown error while parsing IRDB";
    }
  }
};

PSR_CONSTINIT IRDBParsingErrorCategory IRDBParsingErrorCat{};

std::error_code make_error_code(IRDBParsingError Err) noexcept {
  return {int(Err), IRDBParsingErrorCat};
}
} // namespace

} // namespace psr

namespace std {
template <> struct is_error_code_enum<psr::IRDBParsingError> : true_type {};
} // namespace std

namespace psr {

llvm::ErrorOr<std::unique_ptr<llvm::Module>>
LLVMProjectIRDB::getParsedIRModuleOrErr(llvm::MemoryBufferRef IRFileContent,
                                        llvm::LLVMContext &Ctx) noexcept {
  llvm::SMDiagnostic Diag;
  std::unique_ptr<llvm::Module> M = llvm::parseIR(IRFileContent, Diag, Ctx);
  bool BrokenDebugInfo = false;
  if (M == nullptr) {
    Diag.print(nullptr, llvm::errs());
    return IRDBParsingError::CouldNotParse;
  }

  if (llvm::verifyModule(*M, &llvm::errs(), &BrokenDebugInfo)) {
    PHASAR_LOG_LEVEL(ERROR, IRFileContent.getBufferIdentifier()
                                << " could not be parsed correctly!");
    return IRDBParsingError::CouldNotVerify;
  }
  if (BrokenDebugInfo) {
    PHASAR_LOG_LEVEL(WARNING, "Debug info is broken!");
  }

  return M;
}

llvm::ErrorOr<std::unique_ptr<llvm::Module>>
LLVMProjectIRDB::getParsedIRModuleOrErr(const llvm::Twine &IRFileName,
                                        llvm::LLVMContext &Ctx) noexcept {
  // Look at LLVM's IRReader.cpp for reference

  auto FileOrErr =
      llvm::MemoryBuffer::getFileOrSTDIN(IRFileName, /*IsText=*/true);
  if (std::error_code EC = FileOrErr.getError()) {
    return FileOrErr.getError();
  }

  return getParsedIRModuleOrErr(*FileOrErr.get(), Ctx);
}

llvm::ErrorOr<LLVMProjectIRDB>
LLVMProjectIRDB::load(const llvm::Twine &IRFileName) {
  auto Ctx = std::make_unique<llvm::LLVMContext>();
  auto M = getParsedIRModuleOrErr(IRFileName, *Ctx);
  if (!M) {
    return M.getError();
  }

  return LLVMProjectIRDB(std::move(*M), std::move(Ctx));
}

LLVMProjectIRDB LLVMProjectIRDB::loadOrExit(const llvm::Twine &IRFileName,
                                            int ErrorExitCode) {
  auto Ret = load(IRFileName);
  if (!Ret) {
    llvm::WithColor::error()
        << "Could not load LLVM-" << LLVM_VERSION_MAJOR << " IR file "
        << IRFileName << ": " << Ret.getError().message() << '\n';
    std::exit(ErrorExitCode);
  }

  return std::move(*Ret);
}

LLVMProjectIRDB::LLVMProjectIRDB(const llvm::Twine &IRFileName)
    : Ctx(new llvm::LLVMContext()) {
  auto M = getParsedIRModuleOrErr(IRFileName, *Ctx);

  if (!M) {
    return;
  }

  auto *NonConst = M->get();
  Mod = std::move(M.get());
  ModulesToSlotTracker::setMSTForModule(Mod.get());
  preprocessModule(NonConst);
}

LLVMProjectIRDB::LLVMProjectIRDB(const llvm::Twine &IRFileName,
                                 bool EnableOpaquePointers)
    : Ctx(new llvm::LLVMContext()) {
  setOpaquePointersForCtx(*Ctx, EnableOpaquePointers);
  auto M = getParsedIRModuleOrErr(IRFileName, *Ctx);

  if (!M) {
    llvm::WithColor::error()
        << "Could not load LLVM-" << LLVM_VERSION_MAJOR << " IR file "
        << IRFileName << ": " << M.getError().message() << '\n';
    return;
  }

  auto *NonConst = M->get();
  Mod = std::move(M.get());
  ModulesToSlotTracker::setMSTForModule(Mod.get());
  preprocessModule(NonConst);
}

void LLVMProjectIRDB::initInstructionIds() {
  assert(Mod != nullptr);
  size_t Id = 0;
  for (auto &Global : Mod->globals()) {
    IdToInst.push_back(&Global);
    InstToId.try_emplace(&Global, Id);

    ++Id;
  }
  IdOffset = Id;

  for (auto &Fun : *Mod) {
    for (auto &Inst : llvm::instructions(Fun)) {
      IdToInst.push_back(&Inst);
      InstToId.try_emplace(&Inst, Id);

      ++Id;
    }
  }

  assert(InstToId.size() == IdToInst.size());
}

/// We really don't need an LLVM Pass for this...
void LLVMProjectIRDB::preprocessModule(llvm::Module *NonConstMod) {
  size_t Id = 0;
  auto &Context = NonConstMod->getContext();
  for (auto &Global : NonConstMod->globals()) {
    llvm::MDNode *Node = llvm::MDNode::get(
        Context, llvm::MDString::get(Context, std::to_string(Id)));
    Global.setMetadata(PhasarConfig::MetaDataKind(), Node);

    IdToInst.push_back(&Global);
    InstToId.try_emplace(&Global, Id);

    ++Id;
  }
  IdOffset = Id;

  for (auto &Fun : *NonConstMod) {
    for (auto &Inst : llvm::instructions(Fun)) {
      llvm::MDNode *Node = llvm::MDNode::get(
          Context, llvm::MDString::get(Context, std::to_string(Id)));
      Inst.setMetadata(PhasarConfig::MetaDataKind(), Node);

      IdToInst.push_back(&Inst);
      InstToId.try_emplace(&Inst, Id);

      ++Id;
    }
  }
  assert(InstToId.size() == IdToInst.size());
}

LLVMProjectIRDB::LLVMProjectIRDB(llvm::Module *Mod, bool DoPreprocessing)
    : Mod(Mod) {
  assert(Mod != nullptr);
  ModulesToSlotTracker::setMSTForModule(Mod);

  if (DoPreprocessing) {
    preprocessModule(Mod);
  } else {
    initInstructionIds();
  }
}

LLVMProjectIRDB::LLVMProjectIRDB(std::unique_ptr<llvm::Module> Mod,
                                 bool DoPreprocessing) {
  assert(Mod != nullptr);
  auto *NonConst = Mod.get();
  ModulesToSlotTracker::setMSTForModule(NonConst);
  this->Mod = std::move(Mod);

  if (DoPreprocessing) {
    preprocessModule(NonConst);
  } else {
    initInstructionIds();
  }
}

LLVMProjectIRDB::LLVMProjectIRDB(std::unique_ptr<llvm::Module> Mod,
                                 std::unique_ptr<llvm::LLVMContext> Ctx,
                                 bool DoPreprocessing)
    : LLVMProjectIRDB(std::move(Mod), DoPreprocessing) {
  this->Ctx = std::move(Ctx);
}

LLVMProjectIRDB::LLVMProjectIRDB(llvm::MemoryBufferRef Buf)
    : Ctx(new llvm::LLVMContext()) {
  auto M = getParsedIRModuleOrErr(Buf, *Ctx);
  if (!M) {
    return;
  }

  auto *NonConst = M->get();
  Mod = std::move(M.get());
  ModulesToSlotTracker::setMSTForModule(Mod.get());
  preprocessModule(NonConst);
}
LLVMProjectIRDB::LLVMProjectIRDB(llvm::MemoryBufferRef Buf,
                                 bool EnableOpaquePointers)
    : Ctx(new llvm::LLVMContext()) {
  setOpaquePointersForCtx(*Ctx, EnableOpaquePointers);
  auto M = getParsedIRModuleOrErr(Buf, *Ctx);
  if (!M) {
    llvm::WithColor::error() << "Could not load " << LLVM_VERSION_MAJOR
                             << " IR buffer: " << Buf.getBufferIdentifier()
                             << ": " << M.getError().message() << '\n';
    return;
  }

  auto *NonConst = M->get();
  Mod = std::move(M.get());
  ModulesToSlotTracker::setMSTForModule(Mod.get());
  preprocessModule(NonConst);
}

LLVMProjectIRDB::~LLVMProjectIRDB() {
  if (Mod) {
    ModulesToSlotTracker::deleteMSTForModule(Mod.get());
  }
}

static llvm::Function *
internalGetFunctionDefinition(const llvm::Module &M,
                              llvm::StringRef FunctionName) {
  auto *F = M.getFunction(FunctionName);
  if (F && !F->isDeclaration()) {
    return F;
  }
  return nullptr;
}

[[nodiscard]] bool LLVMProjectIRDB::debugInfoAvailableImpl() const {
  return Mod->getNamedMetadata("llvm.dbg.cu") != nullptr;
}

/// Non-const overload
[[nodiscard]] llvm::Function *
LLVMProjectIRDB::getFunctionDefinition(llvm::StringRef FunctionName) {
  return internalGetFunctionDefinition(*Mod, FunctionName);
}

[[nodiscard]] const llvm::Function *
LLVMProjectIRDB::getFunctionDefinitionImpl(llvm::StringRef FunctionName) const {
  return internalGetFunctionDefinition(*Mod, FunctionName);
}

[[nodiscard]] const llvm::GlobalVariable *
LLVMProjectIRDB::getGlobalVariableDefinitionImpl(
    llvm::StringRef GlobalVariableName) const {
  auto *G = Mod->getGlobalVariable(GlobalVariableName);
  if (G && !G->isDeclaration()) {
    return G;
  }
  return nullptr;
}

bool LLVMProjectIRDB::isValidImpl() const noexcept { return Mod != nullptr; }

void LLVMProjectIRDB::dumpImpl() const {
  llvm::dbgs() << *Mod;
  llvm::dbgs().flush();
}

void LLVMProjectIRDB::emitPreprocessedIR(llvm::raw_ostream &OS) const {
  assert(isValid());
  struct AAWriter : llvm::AssemblyAnnotationWriter {
    const LLVMProjectIRDB *IRDB{};

    explicit AAWriter(const LLVMProjectIRDB *IRDB) noexcept : IRDB(IRDB) {}

    void printInfoComment(const llvm::Value &V,
                          llvm::formatted_raw_ostream &OS) override {
      if (auto It = IRDB->InstToId.find(&V); It != IRDB->InstToId.end()) {
        OS << "; | ID: " << It->second;
      }
    }
  };

  AAWriter AAW(this);
  Mod->print(OS, &AAW);
}

void LLVMProjectIRDB::insertFunction(llvm::Function *F, bool DoPreprocessing) {
  assert(F->getParent() == Mod.get() &&
         "The new function F should be present in the module of the IRDB!");
  size_t Id = IdToInst.size();

  auto &Context = F->getContext();
  for (auto &Inst : llvm::instructions(F)) {
    if (DoPreprocessing) {
      llvm::MDNode *Node = llvm::MDNode::get(
          Context, llvm::MDString::get(Context, std::to_string(Id)));
      Inst.setMetadata(PhasarConfig::MetaDataKind(), Node);
    }

    IdToInst.push_back(&Inst);
    InstToId.try_emplace(&Inst, Id);

    ++Id;
  }
  assert(InstToId.size() == IdToInst.size());
}

template class ProjectIRDBBase<LLVMProjectIRDB>;

} // namespace psr

const llvm::Value *psr::fromMetaDataId(const LLVMProjectIRDB &IRDB,
                                       llvm::StringRef Id) {
  if (Id.empty() || Id[0] == '-') {
    return nullptr;
  }

  auto ParseInt = [](llvm::StringRef Str) -> std::optional<unsigned> {
    unsigned Num;
    auto [Ptr, EC] = std::from_chars(Str.begin(), Str.end(), Num);

    if (EC == std::errc{}) {
      return Num;
    }

    PHASAR_LOG_LEVEL(WARNING,
                     "Invalid metadata id '"
                         << Str << "': " << std::make_error_code(EC).message());
    return std::nullopt;
  };

  if (auto Dot = Id.find('.'); Dot != llvm::StringRef::npos) {
    auto FName = Id.slice(0, Dot);

    auto ArgNr = ParseInt(Id.drop_front(Dot + 1));

    if (!ArgNr) {
      return nullptr;
    }

    const auto *F = IRDB.getFunction(FName);
    if (F) {
      return getNthFunctionArgument(F, *ArgNr);
    }

    return nullptr;
  }

  auto IdNr = ParseInt(Id);
  return IdNr ? IRDB.getValueFromId(*IdNr) : nullptr;
}
