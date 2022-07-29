#include "phasar/DB/LLVMProjectIRDB.h"
#include "phasar/Config/Configuration.h"
#include "phasar/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"

#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/SourceMgr.h"

namespace psr {

LLVMProjectIRDB::LLVMProjectIRDB(const llvm::Twine &IRFileName) {

  std::unique_ptr<llvm::Module> M = getParsedIRModuleOrNull(IRFileName, Ctx);

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

LLVMProjectIRDB::LLVMProjectIRDB(llvm::Module *Mod) : Mod(Mod) {
  assert(Mod != nullptr);
  ModulesToSlotTracker::setMSTForModule(Mod);
  initInstructionIds();
}

LLVMProjectIRDB::LLVMProjectIRDB(std::unique_ptr<llvm::Module> Mod,
                                 bool DoPreprocessing) {
  assert(Mod != nullptr);
  auto *NonConst = Mod.get();
  ModulesToSlotTracker::setMSTForModule(Mod.get());
  this->Mod = std::move(Mod);

  if (DoPreprocessing) {
    preprocessModule(NonConst);
  } else {
    initInstructionIds();
  }
}

LLVMProjectIRDB::~LLVMProjectIRDB() {
  if (Mod) {
    ModulesToSlotTracker::deleteMSTForModule(Mod.get());
  }
}

std::unique_ptr<llvm::Module>
LLVMProjectIRDB::getParsedIRModuleOrNull(const llvm::Twine &IRFileName,
                                         llvm::LLVMContext &Ctx) noexcept {
  llvm::SMDiagnostic Diag;
  llvm::SmallString<256> Buf;
  std::unique_ptr<llvm::Module> M =
      llvm::parseIRFile(IRFileName.toStringRef(Buf), Diag, Ctx);
  bool BrokenDebugInfo = false;
  if (M == nullptr) {
    Diag.print(nullptr, llvm::errs());
    return nullptr;
  }
  /* Crash in presence of llvm-3.9.1 module (segfault) */
  if (M == nullptr || llvm::verifyModule(*M, &llvm::errs(), &BrokenDebugInfo)) {
    PHASAR_LOG_LEVEL(ERROR, IRFileName << " could not be parsed correctly!");
    return nullptr;
  }
  if (BrokenDebugInfo) {
    PHASAR_LOG_LEVEL(WARNING, "Debug info is broken!");
  }
  return M;
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
  assert(isValid());
  return Mod->getNamedMetadata("llvm.dbg.cu") != nullptr;
}

/// Non-const overload
[[nodiscard]] llvm::Function *
LLVMProjectIRDB::getFunctionDefinition(llvm::StringRef FunctionName) {
  assert(isValid());
  return internalGetFunctionDefinition(*Mod, FunctionName);
}

[[nodiscard]] const llvm::Function *
LLVMProjectIRDB::getFunctionDefinitionImpl(llvm::StringRef FunctionName) const {
  assert(isValid());
  return internalGetFunctionDefinition(*Mod, FunctionName);
}

[[nodiscard]] const llvm::GlobalVariable *
LLVMProjectIRDB::getGlobalVariableDefinitionImpl(
    llvm::StringRef GlobalVariableName) const {
  assert(isValid());
  auto *G = Mod->getGlobalVariable(GlobalVariableName);
  if (G && !G->isDeclaration()) {
    return G;
  }
  return nullptr;
}

bool LLVMProjectIRDB::isValidImpl() const noexcept { return Mod != nullptr; }

void LLVMProjectIRDB::dumpImpl() const {
  if (!isValid()) {
    llvm::dbgs() << "<Invalid Module>\n";
  } else {
    llvm::dbgs() << *Mod;
  }
  llvm::dbgs().flush();
}

} // namespace psr
