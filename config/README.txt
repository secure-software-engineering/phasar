The files glibc_function_list_v1-04.05.17.conf and
llvm_intrinsics_function_list_v1-04.05.17.conf contain a list of all functions
defined in the GNU libc implementation and LLVM intrinsic functions, respectively.
The functions are loaded into Phasar on start. When Phasar analyzes LLVM IR
it frequently observes call-sites that call a function contained in one of those
lists. Following these calls is usually not desired as a user oftentimes do not
want to analyze the next level of lowering (to libc). For pseudo functions like
LLVM intrinsic functions there is no source code to analyze as these functions
are only used to describe semantics (an orthogonal approch to adding a new
instruction); the code generator is responsible to replace the intrinsic
functions with software or hardware implementation for the desired target
architecture. Phasar models calls to these functions as identity (default) by a
user can specify a different behavior if desired.
