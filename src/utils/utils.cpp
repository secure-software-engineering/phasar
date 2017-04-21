#include "utils.hh"

string cxx_demangle(string mangled_name) {
  int status = 0;
  char* demangled = abi::__cxa_demangle(mangled_name.c_str(), NULL, NULL, &status);
  string result((status == 0 && demangled != NULL) ? demangled : mangled_name);
  free(demangled);
  return result;
}

const string MetaDataKind("ourframework.id");

bool isFunctionPointer(const llvm::Value* V) noexcept {
	if (V) {
		if (V->getType()->isPointerTy() && V->getType()->getPointerElementType()->isFunctionTy()) {
			return true;
		}
		return false;
	}
	return false;
}

bool matchesSignature(const llvm::Function* F, const llvm::FunctionType* FType) {
	if (F->getArgumentList().size() == FType->getNumParams() && F->getReturnType() == FType->getReturnType()) {
		auto& arglist = F->getArgumentList();
		bool eq = true;
		unsigned i = 0;
		for (auto& arg : arglist) {
			if (arg.getType() != FType->getParamType(i)) {
				eq = false;
			}
			++i;
		}
		return eq;
	}
	return false;
}
