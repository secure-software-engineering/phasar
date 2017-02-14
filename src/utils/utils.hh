#ifndef UTILS_HH
#define UTILS_HH

#define MYDEBUG

#define HEREANDNOW \
	cerr << "error in file: " << __FILE__ << " line: " << __LINE__ << " function: " << __func__ << endl;

#define CXXERROR(BOOL,STRING) \
	if (!BOOL) { \
		HEREANDNOW; \
		cerr << STRING << endl; \
		exit(-1); \
	} \

#endif
