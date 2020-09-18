/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifdef PHASAR_ENABLE_TAINT_CONFIGURATION_API
#ifndef __clang__
#error Detected invalid compiler. LLVM/Clang compiler is required.
#endif
#include <stdarg.h> // Required for C-style var args.
/// Declares complete function as source. Any inputs and the return value of the
/// given function are considered to be tainted.
extern void phasar_declare_complete_fun_as_source(void *Fun);
#define PHASAR_DECLARE_COMPLETE_FUN_AS_SOURCE(FPTR)                            \
  phasar_declare_complete_fun_as_source((void *)FPTR)
/// Declares inputs and return value of the given function as tainted according
/// to the specific parametrization.
extern void phasar_declare_fun_as_source(void *Fun, bool Ret, ...);
#define PHASAR_DECLARE_FUN_AS_SOURCE(FPTR, Y, ...)                             \
  phasar_declare_fun_as_source((void *)FPTR, Y, __VA_ARGS__)
// Declares complete function as sanitizer. Any inputs to the given function a
// considered to be sanitized (untainted) after a call to the specified
// function.
extern void phasar_declare_complete_fun_as_sanitizer(void *Fun);
#define PHASAR_DECLARE_COMPLETE_FUN_AS_SANITIZER(FPTR)                         \
  phasar_declare_complete_fun_as_sanitizer((void *)FPTR)
/// Declares inputs of the given function as sanitized (untainted) according th
/// the specific parametrization.
extern void phasar_declare_fun_as_sanitizer(void *Fun, ...);
#define PHASAR_DECLARE_FUN_AS_SANITIZER(FPTR, ...)                             \
  phasar_declare_fun_as_sanitizer((void *)FPTR, __VA_ARGS__)
/// Declares complete function as sink. Any tainted inputs to the specified
/// function are considered as leaks.
extern void phasar_declare_complete_fun_as_sink(void *Fun);
#define PHASAR_DECLARE_COMPLETE_FUN_AS_SINK(FPTR)                              \
  phasar_declare_complete_fun_as_sink((void *)FPTR)
/// Declares specific input parameters of the specified function as sinks. If a
/// tainted value it passed as a respective actual argument it is considered a
/// leak.
extern void phasar_declare_fun_as_sink(void *Fun, ...);
#define PHASAR_DECLARE_FUN_AS_SINK(FPTR, ...)                                  \
  phasar_declare_fun_as_sink((void *)FPTR, __VA_ARGS__)
/// Declares the specified variable as tainted.
extern void phasar_declare_var_as_source(void *Var);
#define PHASAR_DECLARE_VAR_AS_SOURCE(VAR) phasar_declare_var_as_source(&VAR)
/// Declares the specified variable as a sink. Whenever a tainted value
/// interacts with the sink variable it is considered a leak.
extern void phasar_declare_var_as_sink(void *Var);
#define PHASAR_DECLARE_VAR_AS_SINK(VAR) phasar_declare_var_as_sink(&VAR)
/// Declares the specified variable as sanitized (untainted).
extern void phasar_declare_var_as_sanitized(void *Var);
#define PHASAR_DECLARE_VAR_AS_SANITIZED(VAR)                                   \
  phasar_declare_var_as_sanitized(&VAR)
#else
// If the preprocessor symbol 'PHASAR_ENABLE_TAINT_CONFIGURATION_API' is not
// defined and the taint configuration API is turned off we replace the macros
// with NOPs such that the compiler eliminates all of our taint annotations.
#define PHASAR_DECLARE_COMPLETE_FUN_AS_SOURCE(FPTR) ((void)0)
#define PHASAR_DECLARE_FUN_AS_SOURCE(FPTR, RET, ...) ((void)0)
#define PHASAR_DECLARE_COMPLETE_FUN_AS_SANITIZER(FPTR) ((void)0)
#define PHASAR_DECLARE_FUN_AS_SANITIZER(FPTR, ...) ((void)0)
#define PHASAR_DECLARE_COMPLETE_FUN_AS_SINK(FPTR) ((void)0)
#define PHASAR_DECLARE_FUN_AS_SINK(FPTR, ...) ((void)0)
#define PHASAR_DECLARE_VAR_AS_SOURCE(VAR) ((void)0)
#define PHASAR_DECLARE_VAR_AS_SINK(VAR) ((void)0)
#define PHASAR_DECLARE_VAR_AS_SANITIZED(VAR) ((void)0)
#endif
