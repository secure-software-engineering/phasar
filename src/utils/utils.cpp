#include "utils.hh"

string cxx_demangle(string mangled_name) {
  int status = 0;
  char* demangled = abi::__cxa_demangle(mangled_name.c_str(), NULL, NULL, &status);
  string result((status == 0 && demangled != NULL) ? demangled : mangled_name);
  free(demangled);
  return result;
}