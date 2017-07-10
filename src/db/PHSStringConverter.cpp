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
  } else if (llvm::isa<llvm::Value>(V)) {
    // In this case we should have an operand of an instruction which can be
    // identified by the instruction id and the operand index.
    cout << "special case: WE ARE AN OPERAND\n";
    // We should only have one user in this special case
    for (auto User : V->users()) {
      if (const llvm::Instruction *I = llvm::dyn_cast<llvm::Instruction>(User)) {
        for (unsigned idx = 0; idx < I->getNumOperands(); ++idx) {
          if (I->getOperand(idx) == V) {
            return I->getFunction()->getName().str() + "." + getMetaDataID(I) + ".o." + to_string(idx);
          }
        } 
      }
    }
    UNRECOVERABLE_CXX_ERROR_UNCOND("llvm::Value is of unexpected type.");
    return "";
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
  } else if (S.find(".o.") != string::npos) {
    unsigned i = S.find(".");
    unsigned j = S.find(".o.");
    unsigned instID = stoi(S.substr(i+1, j));
    // cout << "FOUND instID: " << instID << "\n";
    unsigned opIdx = stoi(S.substr(j+3, S.size()));
    // cout << "FOUND opIdx: " << to_string(opIdx) << "\n";
    llvm::Function* F = IRDB.getFunction(S.substr(0, S.find(".")));
    for (auto& BB : *F) {
      for (auto& I : BB) {
        if (getMetaDataID(&I) == to_string(instID)) {
          return I.getOperand(opIdx);
        }
      }
    }
    UNRECOVERABLE_CXX_ERROR_UNCOND("Operand not found.");
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
