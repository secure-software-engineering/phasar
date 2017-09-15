#include "SOL.hh"

SOL::SOL(const string& path) {
  cout << path.c_str() << endl;
  cout << "TRYING TO LOAD SO LIB" << endl;
  so_handle = dlopen(path.c_str(), RTLD_LAZY); // TODO this line causes crash :-(
  if (!so_handle) {
    cerr << dlerror() << '\n';
    throw runtime_error("could not open shared object library");
  }
  dlerror();  // clear existing errors
}

SOL::~SOL() {
  if ((error = dlerror()) != NULL) {
    cerr << error << '\n';
    throw runtime_error("shared object library has problems");
  }
  dlclose(so_handle);
}