#include "phasar/Utils/LibrarySummary.h"

#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/StringRef.h"

static constexpr llvm::StringLiteral HeapAllocatingFunNames[] = {
    // Also see SVF's extapi.c

    // C functions:
    "malloc",
    "calloc",
    "fopen",
    "fopen64",
    "fdopen",
    "readdir64",
    "tmpvoid64",
    "zmalloc",
    "gzdopen",
    "iconv_open",
    "lalloc",
    "lalloc_clear",
    "nhalloc",
    "oballoc",
    "popen",
    "pthread_getspecific",
    "readdir",
    "safe_calloc",
    "safe_malloc",
    "safecalloc",
    "safemalloc",
    "setmntent",
    "shmat",
    "__sysv_signal",
    "signal",
    "tempnam",
    "tmpvoid",
    "xcalloc",
    "xmalloc",
    "xnmalloc",
    "xcharalloc",
    "xinmalloc",
    "xizalloc",
    "xzalloc",
    "xpalloc",
    "xicalloc",
    "icalloc",
    "imalloc",
    "_gl_alloc_nomem",
    "aligned_alloc",
    "memalign",
    "valloc",
    "mmap64",
    "XSetLocaleModifiers",
    "__strdup",
    "xmemdup",
    "crypt",
    "ctime",
    "dlerror",
    "dlopen",
    "gai_strerror",
    "gcry_cipher_algo_name",
    "svfgcry_md_algo_name_",
    "getenv",
    "getlogin",
    "getpass",
    "gnutls_strerror",
    "gpg_strerror",
    "gzerror",
    "inet_ntoa",
    "initscr",
    "llvm_stacksave",
    "mmap",
    "newwin",
    "nl_langinfo",
    "opendir",
    "sbrk",
    "strdup",
    "xstrdup",
    "strerror",
    "strsignal",
    "textdomain",
    "tgetstr",
    "tigetstr",
    "tmpnam",
    "ttyname",
    "tzalloc",

    // C++ functions:
    "_Znwm",
    "_Znam",
    "_Znwj",
    "_Znaj",
    "_ZnwmRKSt9nothrow_t",
    "_ZnamRKSt9nothrow_t",
    "_ZnwmSt11align_val_t",
    "_ZnamSt11align_val_t",
    "_ZnwmSt11align_val_tRKSt9nothrow_t",
    "_ZnamSt11align_val_tRKSt9nothrow_t",
    "__cxa_allocate_exception",
};

static constexpr llvm::StringLiteral SingletonReturningFunctions[] = {
    "__ctype_b_loc",
    "__ctype_tolower_loc",
    "__ctype_toupper_loc",
    "__errno_location",
    "__h_errno_location",
    "__res_state",
    "asctime",
    "bindtextdomain",
    "bind_textdomain_codeset",
    "dcgettext",
    "dgettext",
    "dngettext",
    "getgrgid",
    "getgrnam",
    "gethostbyaddr",
    "gethostbyname",
    "gethostbyname2",
    "getmntent",
    "getprotobyname",
    "getprotobynumber",
    "getpwent",
    "getpwnam",
    "getpwuid",
    "getservbyname",
    "getservbyport",
    "getspnam",
    "gettext",
    "gmtime",
    "gnu_get_libc_version",
    "gnutls_check_version",
    "localeconv",
    "localtime",
    "ngettext",
    "pango_cairo_font_map_get_default",
    "re_comp",
    "setlocale",
    "tgoto",
    "tparm",
    "zError",
};

bool psr::isHeapAllocatingFunction(llvm::StringRef FName) noexcept {
  // Note: For a performance comparison of different search strategies, see
  // https://quick-bench.com/q/lNjDT6z-M-L08h372fbCfYjwM64

  static const llvm::DenseSet<llvm::StringRef> HAFs(
      std::begin(HeapAllocatingFunNames), std::end(HeapAllocatingFunNames));

  return HAFs.count(FName);
}

bool psr::isSingletonReturningFunction(llvm::StringRef FName) noexcept {
  static const llvm::DenseSet<llvm::StringRef> SRFs(
      std::begin(SingletonReturningFunctions),
      std::end(SingletonReturningFunctions));

  return SRFs.count(FName);
}
