#include "utils.hh"

string cxx_demangle(const string& mangled_name) {
  int status = 0;
  char* demangled = abi::__cxa_demangle(mangled_name.c_str(), NULL, NULL, &status);
  string result((status == 0 && demangled != NULL) ? demangled : mangled_name);
  free(demangled);
  return result;
}

string debasify(const string& name) {
	static const string base = ".base";
	if (boost::algorithm::ends_with(name, base)) {
		return name.substr(0, name.size()-base.size());
	} else {
		return name;
	}
}

bool isMangled(const string& name) {
	return name != cxx_demangle(name);
}

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
		unsigned i = 0;
		for (auto& arg : arglist) {
			if (arg.getType() != FType->getParamType(i)) {
				return false;
			}
			++i;
		}
		return true;
	}
	return false;
}

string llvmIRToString(const llvm::Value* V) {
	string IRBuffer;
	llvm::raw_string_ostream RSO(IRBuffer);
	V->print(RSO);
	RSO.flush();
	return IRBuffer;
}

vector<string> splitString(const string& str, const string& delimiter) {
	vector<string> split_strings;
	boost::split(split_strings, str, boost::is_any_of(delimiter), boost::token_compress_on);
	return split_strings;
}
