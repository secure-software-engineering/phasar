#include "InitSVF.h"

#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/Utils/EmptyBaseOptimizationUtils.h"

#include "SVF-LLVM/LLVMModule.h"
#include "Util/CommandLine.h"
#include "Util/DPItem.h"
#include "Util/Options.h"

static psr::EmptyType initializeSVFImpl() {
  char EmptyStr[] = "";
  char NoAliasCheck[] = "-alias-check=false";
  char NoStat[] = "-stat=false";
  char Extapi[] = "-extapi=" PHASAR_SVF_INSTALL_LIBDIR "/extapi.bc";
  char *MockArgv[] = {
      EmptyStr,
      NoAliasCheck,
      NoStat,
      Extapi,
  };
  OptionBase::parseOptions(std::size(MockArgv), MockArgv, "", "");

  // Initialize these parameters to their default values.
  // See https://github.com/SVF-tools/SVF/blob/master/svf/lib/DDA/DDAPass.cpp
  // for reference
  SVF::ContextCond::setMaxCxtLen(SVF::Options::MaxContextLen());
  SVF::ContextCond::setMaxPathLen(SVF::Options::MaxPathLen());

  return psr::EmptyType{};
}

void psr::initializeSVF() {
  static const auto SVFInitialized = initializeSVFImpl();
  (void)SVFInitialized;
}

void psr::initSVFModule(psr::LLVMProjectIRDB &IRDB) {
  psr::initializeSVF();

  SVF::LLVMModuleSet::buildSVFModule(*IRDB.getModule());
}
