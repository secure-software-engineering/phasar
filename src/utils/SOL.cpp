#include "SOL.hh"

SOL::SOL(const string& path) {
	so_handle = dlopen(path.c_str(), RTLD_LAZY);
	if (!so_handle) {
		cerr << dlerror() << '\n';
		throw runtime_error("could not open shared object library");
	}
	dlerror(); // clear existing errors
}

SOL::~SOL() {
	if ((error = dlerror()) != NULL) {
		cerr << error << '\n';
		throw runtime_error("shared object library has problems");
	}
	dlclose(so_handle);
}