#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"

#include "phasar/Config/Configuration.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"

#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/IR/AssemblyAnnotationWriter.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/MemoryBufferRef.h"
#include "llvm/Support/SourceMgr.h"

#include <charconv>

namespace psr {

std::unique_ptr<llvm::Module>
LLVMProjectIRDB::getParsedIRModuleOrNull(llvm::MemoryBufferRef IRFileContent,
                                         llvm::LLVMContext &Ctx) noexcept {

  llvm::SMDiagnostic Diag;
  std::unique_ptr<llvm::Module> M = llvm::parseIR(IRFileContent, Diag, Ctx);
  bool BrokenDebugInfo = false;
  if (M == nullptr) {
    Diag.print(nullptr, llvm::errs());
    return nullptr;
  }
  /* Crash in presence of llvm-3.9.1 module (segfault) */
  if (M == nullptr || llvm::verifyModule(*M, &llvm::errs(), &BrokenDebugInfo)) {
    PHASAR_LOG_LEVEL(ERROR, IRFileContent.getBufferIdentifier()
                                << " could not be parsed correctly!");
    return nullptr;
  }
  if (BrokenDebugInfo) {
    PHASAR_LOG_LEVEL(WARNING, "Debug info is broken!");
  }
  return M;
}

std::unique_ptr<llvm::Module>
LLVMProjectIRDB::getParsedIRModuleOrNull(const llvm::Twine &IRFileName,
                                         llvm::LLVMContext &Ctx) noexcept {
  // Look at LLVM's IRReader.cpp for reference

  auto FileOrErr =
      llvm::MemoryBuffer::getFileOrSTDIN(IRFileName, /*IsText=*/true);
  if (std::error_code EC = FileOrErr.getError()) {
    llvm::SmallString<128> Buf;
    auto Err = llvm::SMDiagnostic(IRFileName.toStringRef(Buf),
                                  llvm::SourceMgr::DK_Error,
                                  "Could not open input file: " + EC.message());
    Err.print(nullptr, llvm::errs());
    return nullptr;
  }
  return getParsedIRModuleOrNull(*FileOrErr.get(), Ctx);
}

LLVMProjectIRDB::LLVMProjectIRDB(const llvm::Twine &IRFileName) {

  auto M = getParsedIRModuleOrNull(IRFileName, Ctx);

  if (!M) {
    return;
  }

  auto *NonConst = M.get();
  Mod = std::move(M);
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

LLVMProjectIRDB::LLVMProjectIRDB(llvm::MemoryBufferRef Buf) {
  llvm::SMDiagnostic Diag;
  auto M = getParsedIRModuleOrNull(Buf, Ctx);
  if (!M) {
    return;
  }

  auto *NonConst = M.get();
  Mod = std::move(M);
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
