#include "PHSStringConverter.hh"

PHSStringConverter::PHSStringConverter(ProjectIRCompiledDB &IRDB)
    : IRDB(IRDB) {}

string PHSStringConverter::PToHStoreStringRep(const llvm::Value *V) {
	if (isZeroValue(V)) {
		return ZeroValueInternalName;
	} else if (const llvm::Instruction *I = llvm::dyn_cast<llvm::Instruction>(V)) {
    return I->getFunction()->getName().str() + "." + getMetaDataID(I);
  } else if (const llvm::Argument *A = llvm::dyn_cast<llvm::Argument>(V)) {
    return A->getParent()->getName().str() + ".f" + to_string(A->getArgNo());
  } else if (const llvm::GlobalValue *G = llvm::dyn_cast<llvm::GlobalValue>(V)) {
    return G->getName().str();
  } else {
    UNRECOVERABLE_CXX_ERROR_UNCOND("llvm::Value is of unexpected type.");
    return "";
  }
}

const llvm::Value *PHSStringConverter::HStoreStringRepToP(const string &S) {
  if (S == ZeroValueInternalName || S.find(ZeroValueInternalName) != string::npos) {
  	return new ZeroValue;
  } else if (S.find(".") == string::npos) {
    return IRDB.getGlobalVariable(S);
  } else if (S.find(".f") != string::npos) {
    unsigned argno = stoi(S.substr(S.find(".f")+2, S.size()));
    return getNthFunctionArgument(IRDB.getFunction(S.substr(0, S.find(".f"))), argno);
  } else if (S.find(".") != string::npos) {
    llvm::Function* F = IRDB.getFunction(S.substr(0, S.find(".")));
    for (auto& BB : *F) {
    	for (auto& I : BB) {
    		if (getMetaDataID(&I) == S.substr(S.find(".")+1, S.size())) {
    			return &I;
    		}
    	}
    }
    UNRECOVERABLE_CXX_ERROR_UNCOND("llvm::Instruction not found.");
  } else {
    UNRECOVERABLE_CXX_ERROR_UNCOND("string cannot be translated into llvm::Value.");
  }
  return nullptr;
}
