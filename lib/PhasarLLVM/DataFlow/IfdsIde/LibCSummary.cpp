#include "phasar/PhasarLLVM/DataFlow/IfdsIde/LibCSummary.h"

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/FunctionDataFlowFacts.h"

using namespace psr;
using namespace psr::library_summary;

static library_summary::FunctionDataFlowFacts createLibCSummary() {
  FunctionDataFlowFacts Sum;

  // abs
  Sum.addElement("abs", 0, ReturnValue{});

  // acos
  Sum.addElement("acos", 0, ReturnValue{});

  // acosf
  Sum.addElement("acosf", 0, ReturnValue{});

  // acosh
  Sum.addElement("acosh", 0, ReturnValue{});

  // acoshf
  Sum.addElement("acoshf", 0, ReturnValue{});

  // acoshl
  Sum.addElement("acoshl", 0, ReturnValue{});

  // acosl
  Sum.addElement("acosl", 0, ReturnValue{});

  // argz_add
  Sum.addElement("argz_add", 2, Parameter{0});

  // argz_add_sep
  Sum.addElement("argz_add_sep", 2, Parameter{0});

  // argz_append
  Sum.addElement("argz_append", 2, Parameter{0});
  Sum.addElement("argz_append", 3, Parameter{1});

  // agrz_create
  Sum.addElement("argz_create", 0, Parameter{1});

  // argz_create_sep
  Sum.addElement("argz_create_sep", 0, Parameter{2});

  // argz_extract
  Sum.addElement("argz_extract", 0, Parameter{2});

  // argz_insert
  Sum.addElement("argz_insert", 3, Parameter{0});

  // argz_next
  Sum.addElement("argz_next", 0, ReturnValue{});

  // argz_replace
  Sum.addElement("argz_replace", 0, Parameter{0});

  // argz_stringify
  Sum.addElement("argz_stringify", 2, Parameter{0});

  // asin
  Sum.addElement("asin", 0, ReturnValue{});

  // asinf
  Sum.addElement("asinf", 0, ReturnValue{});

  // asinh
  Sum.addElement("asinh", 0, ReturnValue{});

  // asinhf
  Sum.addElement("asinhf", 0, ReturnValue{});

  // asinhl
  Sum.addElement("asinhl", 0, ReturnValue{});

  // asinl
  Sum.addElement("asinl", 0, ReturnValue{});

  // asprintf
  Sum.addElement("asprintf", 1, Parameter{0});
  Sum.addElement("asprintf", 2, Parameter{0});
  Sum.addElement("asprintf", 3, Parameter{0});
  Sum.addElement("asprintf", 4, Parameter{0});
  Sum.addElement("asprintf", 5, Parameter{0});

  // atan
  Sum.addElement("atan", 0, ReturnValue{});

  // atan2
  Sum.addElement("atan2", 0, ReturnValue{});
  Sum.addElement("atan2", 1, ReturnValue{});

  // atan2f
  Sum.addElement("atan2f", 0, ReturnValue{});
  Sum.addElement("atan2f", 1, ReturnValue{});

  // atan2l
  Sum.addElement("atan2l", 0, ReturnValue{});
  Sum.addElement("atan2l", 1, ReturnValue{});

  // atanf
  Sum.addElement("atanf", 0, ReturnValue{});

  // atanh
  Sum.addElement("atanh", 0, ReturnValue{});

  // atanhf
  Sum.addElement("atanhf", 0, ReturnValue{});

  // atanhl
  Sum.addElement("atanhl", 0, ReturnValue{});

  // atanl
  Sum.addElement("atanl", 0, ReturnValue{});

  // basename
  Sum.addElement("basename", 0, ReturnValue{});

  // bcopy
  Sum.addElement("bcopy", 0, Parameter{1});

  // bindtextdomain
  Sum.addElement("bindtextdomain", 1, ReturnValue{});

  // bind_textdomain_codeset
  Sum.addElement("bind_textdomain_codeset", 1, ReturnValue{});

  // bsearch
  Sum.addElement("bsearch", 1, ReturnValue{});

  // btowc
  Sum.addElement("btowc", 0, ReturnValue{});

  // cabs
  Sum.addElement("cabs", 0, ReturnValue{});

  // cabsf
  Sum.addElement("cabsf", 0, ReturnValue{});

  // cabsl
  Sum.addElement("cabsl", 0, ReturnValue{});

  // cacos
  Sum.addElement("cacos", 0, ReturnValue{});

  // cacosf
  Sum.addElement("cacosf", 0, ReturnValue{});

  // cacosl
  Sum.addElement("cacosl", 0, ReturnValue{});

  // cacosh
  Sum.addElement("cacosh", 0, ReturnValue{});

  // cacoshf
  Sum.addElement("cacoshf", 0, ReturnValue{});

  // cacoshl
  Sum.addElement("cacoshl", 0, ReturnValue{});

  // carg
  Sum.addElement("carg", 0, ReturnValue{});

  // cargf
  Sum.addElement("cargf", 0, ReturnValue{});

  // cargl
  Sum.addElement("cargl", 0, ReturnValue{});

  // casin
  Sum.addElement("casin", 0, ReturnValue{});

  // casinf
  Sum.addElement("casinf", 0, ReturnValue{});

  // casinh
  Sum.addElement("casinh", 0, ReturnValue{});

  // casinhf
  Sum.addElement("casinhf", 0, ReturnValue{});

  // casinhl
  Sum.addElement("casinhl", 0, ReturnValue{});

  // casinl
  Sum.addElement("casinl", 0, ReturnValue{});

  // catan
  Sum.addElement("catan", 0, ReturnValue{});

  // catanf
  Sum.addElement("catanf", 0, ReturnValue{});

  // catanh
  Sum.addElement("catanh", 0, ReturnValue{});

  // catanhf
  Sum.addElement("catanhf", 0, ReturnValue{});

  // catanhl
  Sum.addElement("catanhl", 0, ReturnValue{});

  // catanl
  Sum.addElement("catanl", 0, ReturnValue{});

  // catgets
  Sum.addElement("catgets", 3, ReturnValue{});

  // cbrt
  Sum.addElement("cbrt", 0, ReturnValue{});

  // cbrtf
  Sum.addElement("cbrtf", 0, ReturnValue{});

  // cbrtl
  Sum.addElement("cbrtl", 0, ReturnValue{});

  // ccos
  Sum.addElement("ccos", 0, ReturnValue{});

  // ccosf
  Sum.addElement("ccosf", 0, ReturnValue{});

  // ccosh
  Sum.addElement("ccosh", 0, ReturnValue{});

  // ccoshf
  Sum.addElement("ccoshf", 0, ReturnValue{});

  // ccoshl
  Sum.addElement("ccoshl", 0, ReturnValue{});

  // ccosl
  Sum.addElement("ccosl", 0, ReturnValue{});

  // ceil
  Sum.addElement("ceil", 0, ReturnValue{});

  // ceilf
  Sum.addElement("ceilf", 0, ReturnValue{});

  // ceill
  Sum.addElement("ceill", 0, ReturnValue{});

  // cexp
  Sum.addElement("cexp", 0, ReturnValue{});

  // cexpf
  Sum.addElement("cexpf", 0, ReturnValue{});

  // cexpl
  Sum.addElement("cexpl", 0, ReturnValue{});

  // cfgetispeed
  Sum.addElement("cfgetispeed", 0, ReturnValue{});

  // cfgetospeed
  Sum.addElement("cfgetospeed", 0, ReturnValue{});

  // cimag
  Sum.addElement("cimag", 0, ReturnValue{});

  // cimagf
  Sum.addElement("cimagf", 0, ReturnValue{});

  // cimagl
  Sum.addElement("cimagl", 0, ReturnValue{});

  // clog
  Sum.addElement("clog", 0, ReturnValue{});

  // clog10
  Sum.addElement("clog10", 0, ReturnValue{});

  // clog10f
  Sum.addElement("clog10f", 0, ReturnValue{});

  // clog10l
  Sum.addElement("clog10l", 0, ReturnValue{});

  // clogf
  Sum.addElement("clogf", 0, ReturnValue{});

  // clogl
  Sum.addElement("clogl", 0, ReturnValue{});

  // conj
  Sum.addElement("conj", 0, ReturnValue{});

  // conjf
  Sum.addElement("conjf", 0, ReturnValue{});

  // conjl
  Sum.addElement("conjl", 0, ReturnValue{});

  // copysign
  Sum.addElement("copysign", 0, ReturnValue{});
  Sum.addElement("copysign", 1, ReturnValue{});

  // copysignf
  Sum.addElement("copysignf", 0, ReturnValue{});
  Sum.addElement("copysign", 1, ReturnValue{});

  // copysignl
  Sum.addElement("copysignl", 0, ReturnValue{});
  Sum.addElement("copysignl", 1, ReturnValue{});

  // cos
  Sum.addElement("cos", 0, ReturnValue{});

  // cosf
  Sum.addElement("cosf", 0, ReturnValue{});

  // cosh
  Sum.addElement("cosh", 0, ReturnValue{});

  // coshf
  Sum.addElement("coshf", 0, ReturnValue{});

  // coshl
  Sum.addElement("coshl", 0, ReturnValue{});

  // cosl
  Sum.addElement("cosl", 0, ReturnValue{});

  // cpow
  Sum.addElement("cpow", 0, ReturnValue{});
  Sum.addElement("cpow", 1, ReturnValue{});

  // cpowf
  Sum.addElement("cpowf", 0, ReturnValue{});
  Sum.addElement("cpowf", 1, ReturnValue{});

  // cpowl
  Sum.addElement("cpowl", 0, ReturnValue{});
  Sum.addElement("cpowl", 1, ReturnValue{});

  // cproj
  Sum.addElement("cproj", 0, ReturnValue{});

  // cprojf
  Sum.addElement("cproj", 0, ReturnValue{});

  // cprojl
  Sum.addElement("cprojl", 0, ReturnValue{});

  // creal
  Sum.addElement("creal", 0, ReturnValue{});

  // crealf
  Sum.addElement("crealf", 0, ReturnValue{});

  // creall
  Sum.addElement("creall", 0, ReturnValue{});

  // crypt
  Sum.addElement("crypt", 0, ReturnValue{});
  // Sum.addElement("crypt", 1, ReturnValue{});

  // crypt_r
  Sum.addElement("crypt_r", 0, ReturnValue{});
  // Sum.addElement("crypt_r", 1, ReturnValue{});

  // csin
  Sum.addElement("csin", 0, ReturnValue{});

  // csinf
  Sum.addElement("csinf", 0, ReturnValue{});

  // csinh
  Sum.addElement("csinh", 0, ReturnValue{});

  // csinhf
  Sum.addElement("csinhf", 0, ReturnValue{});

  // csinhl
  Sum.addElement("csinhl", 0, ReturnValue{});

  // csinl
  Sum.addElement("csinl", 0, ReturnValue{});

  // csprt
  Sum.addElement("csqrt", 0, ReturnValue{});

  // csqrtf
  Sum.addElement("csqrtf", 0, ReturnValue{});

  // csqrtl
  Sum.addElement("csqrtl", 0, ReturnValue{});

  // ctan
  Sum.addElement("ctan", 0, ReturnValue{});

  // ctanf
  Sum.addElement("ctanf", 0, ReturnValue{});

  // ctanh
  Sum.addElement("ctanh", 0, ReturnValue{});

  // ctanhf
  Sum.addElement("ctanhf", 0, ReturnValue{});

  // ctanhl
  Sum.addElement("ctanhl", 0, ReturnValue{});

  // ctanl
  Sum.addElement("ctanl", 0, ReturnValue{});

  // ctermid
  Sum.addElement("ctermid", 0, ReturnValue{});

  // ctime
  Sum.addElement("ctime", 0, ReturnValue{}); //?

  // ctime_r
  Sum.addElement("ctime_r", 0, Parameter{1});

  // cuserid
  Sum.addElement("cuserid", 0, ReturnValue{});

  // dcgettext
  Sum.addElement("dcgettext", 1, ReturnValue{});

  // dcngettext
  Sum.addElement("dcngettext", 1, ReturnValue{});

  // dgettext
  Sum.addElement("dgettext", 1, ReturnValue{});

  // difftime
  Sum.addElement("difftime", 0, ReturnValue{});
  Sum.addElement("difftime", 1, ReturnValue{});

  // dirname
  Sum.addElement("dirname", 0, ReturnValue{});

  // div
  Sum.addElement("div", 0, ReturnValue{});
  Sum.addElement("div", 1, ReturnValue{});

  // dngettext
  Sum.addElement("dngettext", 1, ReturnValue{});

  // drem
  Sum.addElement("drem", 0, ReturnValue{});
  Sum.addElement("drem", 1, ReturnValue{});

  // dremf
  Sum.addElement("dremf", 0, ReturnValue{});
  Sum.addElement("dremf", 1, ReturnValue{});

  // dreml
  Sum.addElement("dreml", 0, ReturnValue{});
  Sum.addElement("dreml", 1, ReturnValue{});

  // dup
  Sum.addElement("dup", 0, ReturnValue{});

  // dup2
  Sum.addElement("dup2", 0, ReturnValue{});

  // envz_add
  Sum.addElement("envz_add", 2, Parameter{0});
  Sum.addElement("envz_add", 3, Parameter{0});

  // envz_entry
  Sum.addElement("envz_entry", 0, ReturnValue{});

  // envz_get
  Sum.addElement("envz_get", 0, ReturnValue{});

  // envz_merge
  Sum.addElement("envz_merge", 2, ReturnValue{});

  // erf
  Sum.addElement("erf", 0, ReturnValue{});

  // erfc
  Sum.addElement("erfc", 0, ReturnValue{});

  // erfcf
  Sum.addElement("erfcf", 0, ReturnValue{});

  // erfcl
  Sum.addElement("erfcf", 0, ReturnValue{});

  // erff
  Sum.addElement("erff", 0, ReturnValue{});

  // erfl
  Sum.addElement("erfl", 0, ReturnValue{});

  // exp
  Sum.addElement("exp", 0, ReturnValue{});

  // exp10
  Sum.addElement("exp10", 0, ReturnValue{});

  // exp10f
  Sum.addElement("exp10f", 0, ReturnValue{});

  // exp10l
  Sum.addElement("exp10l", 0, ReturnValue{});

  // exp2
  Sum.addElement("exp2", 0, ReturnValue{});

  // exp2f
  Sum.addElement("exp2f", 0, ReturnValue{});

  // exp2l
  Sum.addElement("exp2l", 0, ReturnValue{});

  // expf
  Sum.addElement("expf", 0, ReturnValue{});

  // expl
  Sum.addElement("expl", 0, ReturnValue{});

  // expm1
  Sum.addElement("expm1", 0, ReturnValue{});

  // expm1f
  Sum.addElement("expm1f", 0, ReturnValue{});

  // expm1l
  Sum.addElement("expm1l", 0, ReturnValue{});

  // fabs
  Sum.addElement("fabs", 0, ReturnValue{});

  // fabsf
  Sum.addElement("fabsf", 0, ReturnValue{});

  // fabsl
  Sum.addElement("fabsl", 0, ReturnValue{});

  // fdim
  Sum.addElement("fdim", 0, ReturnValue{});

  // fdimf
  Sum.addElement("fdimf", 0, ReturnValue{});

  // fdiml
  Sum.addElement("fdiml", 0, ReturnValue{});

  // fgetc
  Sum.addElement("fgetc", 0, ReturnValue{});

  // fgetpwent
  Sum.addElement("fgetpwent", 0, ReturnValue{});

  // fgetpwent_f
  Sum.addElement("fgetpwent_r", 0, Parameter{1});

  // fgets
  Sum.addElement("fgets", 2, Parameter{0});
  Sum.addElement("fgets", 0, ReturnValue{});

  // fgetwc
  Sum.addElement("fgetwc", 0, ReturnValue{});

  // fgetws
  Sum.addElement("fgetws", 2, Parameter{0});
  Sum.addElement("fgetws", 0, ReturnValue{});

  // finite
  Sum.addElement("finite", 0, ReturnValue{});

  // finitef
  Sum.addElement("finitef", 0, ReturnValue{});

  // finitel
  Sum.addElement("finitel", 0, ReturnValue{});

  // floor
  Sum.addElement("floor", 0, ReturnValue{});

  // floorf
  Sum.addElement("floorf", 0, ReturnValue{});

  // floorl
  Sum.addElement("floorl", 0, ReturnValue{});

  // fma
  Sum.addElement("fma", 0, ReturnValue{});
  Sum.addElement("fma", 1, ReturnValue{});
  Sum.addElement("fma", 2, ReturnValue{});

  // fmaf
  Sum.addElement("fmaf", 0, ReturnValue{});
  Sum.addElement("fmaf", 1, ReturnValue{});
  Sum.addElement("fmaf", 2, ReturnValue{});

  // fmal
  Sum.addElement("fmal", 0, ReturnValue{});
  Sum.addElement("fmal", 1, ReturnValue{});
  Sum.addElement("fmal", 2, ReturnValue{});

  // fmax
  Sum.addElement("fmax", 0, ReturnValue{});
  Sum.addElement("fmax", 1, ReturnValue{});

  // fmaxf
  Sum.addElement("fmaxf", 0, ReturnValue{});
  Sum.addElement("fmaxf", 1, ReturnValue{});

  // fmaxl
  Sum.addElement("fmaxl", 0, ReturnValue{});
  Sum.addElement("fmaxl", 1, ReturnValue{});

  // fmaxmag
  Sum.addElement("fmaxmag", 0, ReturnValue{});
  Sum.addElement("fmaxmag", 1, ReturnValue{});

  // fmaxmagf
  Sum.addElement("fmaxmag", 0, ReturnValue{});
  Sum.addElement("fmaxmagf", 1, ReturnValue{});

  // fmaxmagl
  Sum.addElement("fmaxmagl", 0, ReturnValue{});
  Sum.addElement("fmaxmag", 1, ReturnValue{});

  // fmin
  Sum.addElement("fmin", 0, ReturnValue{});
  Sum.addElement("fmin", 1, ReturnValue{});

  // fminf
  Sum.addElement("fminf", 0, ReturnValue{});
  Sum.addElement("fminf", 1, ReturnValue{});

  // fminl
  Sum.addElement("fminl", 0, ReturnValue{});
  Sum.addElement("fminl", 1, ReturnValue{});

  // fminmag
  Sum.addElement("fminmag", 0, ReturnValue{});
  Sum.addElement("fminmag", 1, ReturnValue{});

  // fminmagf
  Sum.addElement("fminmagf", 0, ReturnValue{});
  Sum.addElement("fminmagf", 1, ReturnValue{});

  // fminmagl
  Sum.addElement("fminmagl", 0, ReturnValue{});
  Sum.addElement("fminmagl", 1, ReturnValue{});

  // fmod
  Sum.addElement("fmod", 0, ReturnValue{});
  Sum.addElement("fmod", 1, ReturnValue{});

  // fmodf
  Sum.addElement("fmodf", 0, ReturnValue{});
  Sum.addElement("fmodf", 1, ReturnValue{});

  // fmodl
  Sum.addElement("fmodl", 0, ReturnValue{});
  Sum.addElement("fmodl", 1, ReturnValue{});

  // fprintf
  Sum.addElement("fprintf", 1, Parameter{0});
  Sum.addElement("fprintf", 2, Parameter{0});
  Sum.addElement("fprintf", 3, Parameter{0});
  //....

  // fputc
  Sum.addElement("fputc", 0, Parameter{1});

  // fputs
  Sum.addElement("fputs", 0, Parameter{1});

  // fputwc
  Sum.addElement("fputwc", 0, Parameter{1});

  // fputws
  Sum.addElement("fputws", 0, Parameter{1});

  // fread
  Sum.addElement("fread", 3, Parameter{0});

  // frexp
  Sum.addElement("frexp", 0, Parameter{1});
  Sum.addElement("frexp", 0, ReturnValue{});

  // frexpf
  Sum.addElement("frexpf", 0, Parameter{1});
  Sum.addElement("frexpf", 0, ReturnValue{});

  // frexpl
  Sum.addElement("frexpl", 0, Parameter{1});
  Sum.addElement("frexpl", 0, ReturnValue{});

  // fromfp
  Sum.addElement("fromfp", 0, ReturnValue{});

  // fromfpf
  Sum.addElement("fromfpf", 0, ReturnValue{});

  // fromfpl
  Sum.addElement("fromfpl", 0, ReturnValue{});

  // fromfpx
  Sum.addElement("fromfpx", 0, ReturnValue{});

  // fromfpxf
  Sum.addElement("fromfpxf", 0, ReturnValue{});

  // fromfpxl
  Sum.addElement("fromfpxl", 0, ReturnValue{});

  // fscanf
  Sum.addElement("fscanf", 0, Parameter{2});

  // fstat
  Sum.addElement("fstat", 0, Parameter{1});

  // fstat64
  Sum.addElement("fstat64", 0, Parameter{});

  // fwprintf
  Sum.addElement("fwprintf", 1, Parameter{0});
  Sum.addElement("fwprintf", 2, Parameter{0});
  Sum.addElement("fwprintf", 3, Parameter{0});

  // fwrite
  Sum.addElement("fwrite", 0, Parameter{3});

  // fwscanf
  Sum.addElement("fwscanf", 0, Parameter{2});

  // gamma
  Sum.addElement("gamma", 0, ReturnValue{});

  // gammaf
  Sum.addElement("gammaf", 0, ReturnValue{});

  // gammal
  Sum.addElement("gammal", 0, ReturnValue{});

  // gcvt
  Sum.addElement("gcvt", 0, Parameter{2});
  Sum.addElement("gcvt", 2, ReturnValue{});

  // getauxval
  Sum.addElement("getauxval", 0, ReturnValue{});

  // getc
  Sum.addElement("getc", 0, ReturnValue{});

  // getc_unlocked
  Sum.addElement("getc_unlocked", 0, ReturnValue{});

  // getchar
  Sum.addElement("getchar", 0, ReturnValue{});

  // getchar_unlocked
  Sum.addElement("getchar_unlocked", 0, ReturnValue{});

  // getcwd
  Sum.addElement("getcwd", 0, ReturnValue{});

  // getdate
  Sum.addElement("getdate", 0, ReturnValue{});

  // getdate_r
  Sum.addElement("getdate_r", 0, Parameter{1});

  // getdelim
  Sum.addElement("getdelim", 3, Parameter{0});

  // getline
  Sum.addElement("getline", 2, Parameter{0});

  // getpayload
  Sum.addElement("getpayload", 0, ReturnValue{});

  // getpayloadf
  Sum.addElement("getpayloadf", 0, ReturnValue{});

  // getpayloadl
  Sum.addElement("getpayloadl", 0, ReturnValue{});

  // getpeername
  Sum.addElement("getpeername", 0, Parameter{});

  // getrlimit
  Sum.addElement("getrlimit", 1, ReturnValue{});

  // gets
  Sum.addElement("gets", 0, ReturnValue{});

  // gettext
  Sum.addElement("gettext", 0, ReturnValue{});

  // gettimeofday
  Sum.addElement("gettimeofday", 0, Parameter{1});

  // getutent_r
  Sum.addElement("getutent_r", 0, Parameter{1});

  // getutid
  Sum.addElement("getutid", 0, ReturnValue{});

  // getutid_r
  Sum.addElement("getutid", 0, Parameter{1});
  Sum.addElement("getutid", 1, Parameter{2});

  // getutline
  Sum.addElement("getutline", 0, ReturnValue{});

  // getutline_r
  Sum.addElement("getutline_r", 0, Parameter{1});
  Sum.addElement("getutline_r", 1, Parameter{2});

  // getutmp
  Sum.addElement("getutmp", 0, Parameter{1});

  // getutmpx
  Sum.addElement("getutmp", 1, Parameter{0});

  // getw
  Sum.addElement("getw", 0, ReturnValue{});

  // getwc
  Sum.addElement("getwc", 0, ReturnValue{});

  // getwc_unlocked
  Sum.addElement("getwc_unlocked", 0, ReturnValue{});

  // getwd
  Sum.addElement("getwd", 0, ReturnValue{});

  // gmtime
  Sum.addElement("gmtime", 0, ReturnValue{});

  // gmtime_r
  Sum.addElement("gmtime_r", 0, Parameter{1});

  // hasmntopt
  Sum.addElement("hasmntopt", 0, Parameter{0});

  // htonl
  Sum.addElement("htonl", 0, ReturnValue{});

  // htons
  Sum.addElement("htons", 0, ReturnValue{});

  // hypot
  Sum.addElement("hypot", 0, ReturnValue{});
  Sum.addElement("hypot", 1, ReturnValue{});

  // hypotf
  Sum.addElement("hypotf", 0, ReturnValue{});
  Sum.addElement("hypotf", 1, ReturnValue{});

  // hypotl
  Sum.addElement("hypotl", 0, ReturnValue{});
  Sum.addElement("hypotl", 1, ReturnValue{});

  // iconv
  Sum.addElement("iconv", 1, Parameter{3});

  // if_indextoname
  Sum.addElement("if_indextoname", 1, ReturnValue{});

  // ilogb
  Sum.addElement("ilogb", 0, ReturnValue{});

  // ilogbf
  Sum.addElement("ilogbf", 0, ReturnValue{});

  // ilogbl
  Sum.addElement("ilogbl", 0, ReturnValue{});

  // imaxabs
  Sum.addElement("imaxabs", 0, ReturnValue{});

  // imaxdiv
  Sum.addElement("imaxdiv", 0, ReturnValue{});
  Sum.addElement("imaxdiv", 1, ReturnValue{});

  // index
  Sum.addElement("index", 0, ReturnValue{});

  // inet_lnaof
  Sum.addElement("inet_lnaof", 0, ReturnValue{});

  // inet_netof
  Sum.addElement("inet_netof", 0, ReturnValue{});

  // inet_network
  Sum.addElement("inet_network", 0, ReturnValue{});

  // inet_ntoa
  Sum.addElement("inet_ntoa", 0, ReturnValue{});

  // inet_ntop
  Sum.addElement("inet_ntop", 1, Parameter{2});
  Sum.addElement("inet_ntop", 2, ReturnValue{});

  // inet_pton
  Sum.addElement("inet_pton", 1, Parameter{2});

  // j0
  Sum.addElement("j0", 0, ReturnValue{});

  // j0f
  Sum.addElement("j0f", 0, ReturnValue{});

  // j0l
  Sum.addElement("j0l", 0, ReturnValue{});

  // j1
  Sum.addElement("j1", 0, ReturnValue{});

  // j1f
  Sum.addElement("j1f", 0, ReturnValue{});

  // j1l
  Sum.addElement("j1l", 0, ReturnValue{});

  // jn
  Sum.addElement("jn", 0, ReturnValue{});
  Sum.addElement("jn", 1, ReturnValue{});

  // jnf
  Sum.addElement("jnf", 0, ReturnValue{});
  Sum.addElement("jnf", 1, ReturnValue{});

  // jnl
  Sum.addElement("jnl", 0, ReturnValue{});
  Sum.addElement("jnl", 1, ReturnValue{});

  // l64a
  Sum.addElement("l64a", 0, ReturnValue{});

  // labs
  Sum.addElement("labs", 0, ReturnValue{});

  // llabs
  Sum.addElement("llabs", 0, ReturnValue{});

  // ldexp
  Sum.addElement("ldexp", 0, ReturnValue{});
  Sum.addElement("ldexp", 1, ReturnValue{});

  // ldexpf
  Sum.addElement("ldexp", 0, ReturnValue{});
  Sum.addElement("ldexp", 1, ReturnValue{});

  // ldexpl
  Sum.addElement("ldexpl", 0, ReturnValue{});
  Sum.addElement("ldexpl", 1, ReturnValue{});

  // ldiv
  Sum.addElement("ldiv", 0, ReturnValue{});
  Sum.addElement("ldiv", 1, ReturnValue{});

  // lfind
  Sum.addElement("lfind", 1, ReturnValue{});

  // lgammaf_r
  Sum.addElement("lgmmaf_r", 0, Parameter{1});

  // lgammal_r
  Sum.addElement("lgammal_r", 0, ReturnValue{});

  // lgamma_r
  Sum.addElement("lgamma_r", 0, Parameter{1});

  // lldiv
  Sum.addElement("lldiv", 0, ReturnValue{});
  Sum.addElement("lldiv", 1, ReturnValue{});

  // llogb
  Sum.addElement("llogb", 0, ReturnValue{});

  // llogbf
  Sum.addElement("llogbf", 0, ReturnValue{});

  // llogbl
  Sum.addElement("llogbl", 0, ReturnValue{});

  // llrint
  Sum.addElement("llrint", 0, ReturnValue{});

  // llrintf
  Sum.addElement("llrintf", 0, ReturnValue{});

  // llrintl
  Sum.addElement("llrinf", 0, ReturnValue{});

  // llround
  Sum.addElement("llround", 0, ReturnValue{});

  // llroundf
  Sum.addElement("llroundf", 0, ReturnValue{});

  // llroundl
  Sum.addElement("llroundl", 0, ReturnValue{});

  // localtime
  Sum.addElement("localtime", 0, ReturnValue{});

  // localtime_r
  Sum.addElement("localtime_r", 0, Parameter{1});
  Sum.addElement("localtime_r", 1, ReturnValue{});

  // log
  Sum.addElement("log", 0, ReturnValue{});

  // log10
  Sum.addElement("log10", 0, ReturnValue{});

  // log10f
  Sum.addElement("log10f", 0, ReturnValue{});

  // log10l
  Sum.addElement("log10l", 0, ReturnValue{});

  // log1p
  Sum.addElement("log1p", 0, ReturnValue{});

  // log1pf
  Sum.addElement("log1pf", 0, ReturnValue{});

  // log1pl
  Sum.addElement("log1pl", 0, ReturnValue{});

  // log2
  Sum.addElement("log2", 0, ReturnValue{});

  // log2f
  Sum.addElement("log2f", 0, ReturnValue{});

  // log2l
  Sum.addElement("log2l", 0, ReturnValue{});

  // logb
  Sum.addElement("logb", 0, ReturnValue{});

  // logbf
  Sum.addElement("logbf", 0, ReturnValue{});

  // logbl
  Sum.addElement("logbl", 0, ReturnValue{});

  // logf
  Sum.addElement("logf", 0, ReturnValue{});

  // logl
  Sum.addElement("logl", 0, ReturnValue{});

  // lrint
  Sum.addElement("lrint", 0, ReturnValue{});

  // lrintf
  Sum.addElement("lrintf", 0, ReturnValue{});

  // lrintl
  Sum.addElement("lrintl", 0, ReturnValue{});

  // lround
  Sum.addElement("lround", 0, ReturnValue{});

  // lroundf
  Sum.addElement("lroundf", 0, ReturnValue{});

  // lroundl
  Sum.addElement("lroundl", 0, ReturnValue{});

  // lsearch
  Sum.addElement("lsearch", 1, ReturnValue{});
  Sum.addElement("lsearch", 0, Parameter{});

  // lstat
  Sum.addElement("lstat", 0, Parameter{1});

  // lstat64
  Sum.addElement("lstat64", 0, Parameter{1});

  // lutimes
  Sum.addElement("lutimes", 1, Parameter{0});

  // mbrtowc
  Sum.addElement("mbrtowc", 1, Parameter{0});

  // mbsnrtowcs
  Sum.addElement("mbsnrtowcs", 1, Parameter{0});

  // mbsrtowcs
  Sum.addElement("mbsrtowcs", 1, Parameter{0});

  // mbstowcs
  Sum.addElement("mbstowcs", 1, Parameter{0});

  // memccpy
  Sum.addElement("memccpy", 1, Parameter{0});

  // memcpy
  Sum.addElement("memcpy", 1, Parameter{0});

  // memfrob
  Sum.addElement("memfrob", 0, ReturnValue{});

  // memmem
  Sum.addElement("memmem", 0, ReturnValue{});

  // memmove
  Sum.addElement("memmove", 1, Parameter{});
  Sum.addElement("memmove", 0, ReturnValue{});

  // mempcpy
  Sum.addElement("mempcpy", 1, Parameter{0});
  Sum.addElement("mempcpy", 1, ReturnValue{});

  // memchr
  Sum.addElement("memchr", 0, ReturnValue{});

  // memrchr
  Sum.addElement("memrchr", 0, ReturnValue{});

  // memset
  Sum.addElement("memset", 1, Parameter{0});
  Sum.addElement("memset", 0, ReturnValue{});

  // mkdtemp
  Sum.addElement("mkdtemp", 0, ReturnValue{});

  // mktemp
  Sum.addElement("mktemp", 0, ReturnValue{});

  // mktime
  Sum.addElement("mktime", 0, ReturnValue{});

  // modf
  Sum.addElement("modf", 0, Parameter{1});
  Sum.addElement("modf", 0, ReturnValue{});

  // modff
  Sum.addElement("modff", 0, Parameter{1});
  Sum.addElement("modff", 0, ReturnValue{});

  // modfl
  Sum.addElement("modfl", 0, Parameter{1});
  Sum.addElement("modfl", 0, ReturnValue{});

  // mount
  Sum.addElement("mount", 0, Parameter{});

  // mremap
  Sum.addElement("mremap", 0, ReturnValue{});
  Sum.addElement("mremap", 4, ReturnValue{});

  // nan
  Sum.addElement("nan", 0, ReturnValue{});

  // nanf
  Sum.addElement("nanf", 0, ReturnValue{});

  // nanl
  Sum.addElement("nanl", 0, ReturnValue{});

  // nearbyint
  Sum.addElement("nearbyint", 0, ReturnValue{});

  // nearbyintf
  Sum.addElement("nearbyintf", 0, ReturnValue{});

  // nearbyintl
  Sum.addElement("nearbyintl", 0, ReturnValue{});

  // nextafter
  Sum.addElement("nextafter", 0, ReturnValue{});

  // nextafterf
  Sum.addElement("nextafterl", 0, ReturnValue{});

  // nextafterl
  Sum.addElement("nextafterl", 0, ReturnValue{});

  // nextdown
  Sum.addElement("nextdown", 0, ReturnValue{});

  // nextdownf
  Sum.addElement("nextdownf", 0, ReturnValue{});

  // nextdownl
  Sum.addElement("nextdownl", 0, ReturnValue{});

  // nexttoward
  Sum.addElement("nexttoward", 0, ReturnValue{});

  // nexttowardf
  Sum.addElement("nexttowardf", 0, ReturnValue{});

  // nexttowardl
  Sum.addElement("nexttowardl", 0, ReturnValue{});

  // nextup
  Sum.addElement("nextup", 0, ReturnValue{});

  // nextupf
  Sum.addElement("nextupf", 0, ReturnValue{});

  // nextupl
  Sum.addElement("nextupl", 0, ReturnValue{});

  // ngettext
  Sum.addElement("ngettext", 0, ReturnValue{});

  // nice
  Sum.addElement("nice", 0, ReturnValue{});

  // nl_langinfo
  Sum.addElement("nl_langinfo", 0, ReturnValue{});

  // ntohl
  Sum.addElement("ntohl", 0, ReturnValue{});

  // ntohs
  Sum.addElement("ntohs", 0, ReturnValue{});

  // pow
  Sum.addElement("pow", 0, ReturnValue{});
  Sum.addElement("pow", 1, ReturnValue{});

  // pow10
  Sum.addElement("pow10", 0, ReturnValue{});

  // pow10f
  Sum.addElement("powf", 0, ReturnValue{});

  // pow10l
  Sum.addElement("pow10l", 0, ReturnValue{});

  // powf
  Sum.addElement("powf", 0, ReturnValue{});
  Sum.addElement("powf", 1, ReturnValue{});

  // powl
  Sum.addElement("powl", 0, ReturnValue{});
  Sum.addElement("powl", 1, ReturnValue{});

  // pread
  Sum.addElement("pread", 0, Parameter{});

  // pread64
  Sum.addElement("pread64", 0, Parameter{});

  // ptsname_r
  Sum.addElement("ptsname_r", 0, Parameter{1});

  // putc
  Sum.addElement("putc", 0, Parameter{1});

  // putc_unlocked
  Sum.addElement("putc_unlocked", 0, Parameter{1});

  // putpwent
  Sum.addElement("putpwent", 0, Parameter{1});

  // pututline
  Sum.addElement("pututline", 0, ReturnValue{});

  // putw
  Sum.addElement("putw", 0, ReturnValue{});

  // putwc
  Sum.addElement("putwc", 0, Parameter{1});

  // putwc_unlocked
  Sum.addElement("putwc_unlocked", 0, Parameter{1});

  // pwrite
  Sum.addElement("pwrite", 1, Parameter{0});

  // pwrite64
  Sum.addElement("pwrite64", 1, Parameter{0});

  // qecvt
  Sum.addElement("qecvt", 0, ReturnValue{});
  Sum.addElement("qecvt", 0, Parameter{3});
  Sum.addElement("qecvt", 0, Parameter{2});

  // qecvt_r
  Sum.addElement("qecvt_r", 0, Parameter{4});
  Sum.addElement("qecvt_r", 0, Parameter{3});
  Sum.addElement("qecvt_r", 0, Parameter{2});

  // qfcvt
  Sum.addElement("qfcvt", 0, ReturnValue{});
  Sum.addElement("qfcvt", 0, Parameter{3});
  Sum.addElement("qfcvt", 0, Parameter{2});

  // qfcvt_r
  Sum.addElement("qfcvt_r", 0, Parameter{4});
  Sum.addElement("qfcvt_r", 0, Parameter{3});
  Sum.addElement("qfcvt_r", 0, Parameter{2});

  // qgcvt
  Sum.addElement("qgcvt", 0, Parameter{2});
  Sum.addElement("qgcvt", 2, ReturnValue{});

  // rawmemchr
  Sum.addElement("rawmemchr", 0, ReturnValue{});

  // read
  Sum.addElement("read", 0, Parameter{1});

  // readdir
  Sum.addElement("readdir", 0, ReturnValue{});

  // readdir_r
  Sum.addElement("readdrir_r", 0, Parameter{1});
  Sum.addElement("readdir_r", 1, Parameter{2});

  // readlink
  Sum.addElement("readlink", 0, Parameter{1});

  // readv
  Sum.addElement("readv", 0, Parameter{1});

  // realloc
  Sum.addElement("realloc", 0, ReturnValue{});

  // realpath
  Sum.addElement("realpath", 0, Parameter{});
  Sum.addElement("realpath", 1, ReturnValue{});

  // regcomp
  Sum.addElement("regcomp", 1, Parameter{});

  // regerror
  Sum.addElement("regerror", 0, Parameter{2});
  Sum.addElement("regerror", 1, Parameter{});

  // remainder
  Sum.addElement("remainder", 0, ReturnValue{});
  Sum.addElement("remainder", 1, ReturnValue{});

  // remainderf
  Sum.addElement("remainderf", 0, ReturnValue{});
  Sum.addElement("remainderf", 1, ReturnValue{});

  // remainderl
  Sum.addElement("remainderl", 0, ReturnValue{});
  Sum.addElement("remainderl", 1, ReturnValue{});

  // rindex
  Sum.addElement("rindex", 0, ReturnValue{});

  // rint
  Sum.addElement("rint", 0, ReturnValue{});

  // rintf
  Sum.addElement("rintf", 0, ReturnValue{});

  // rintl
  Sum.addElement("rintl", 0, ReturnValue{});

  // round
  Sum.addElement("round", 0, ReturnValue{});

  // roundeven
  Sum.addElement("roundeven", 0, ReturnValue{});

  // roundevenf
  Sum.addElement("roundevenf", 0, ReturnValue{});

  // roundevenl
  Sum.addElement("roundevenl", 0, ReturnValue{});

  // roundf
  Sum.addElement("roundf", 0, ReturnValue{});

  // roundl
  Sum.addElement("roundl", 0, ReturnValue{});

  // rpmatch
  Sum.addElement("rpmatch", 0, ReturnValue{});

  // scalb
  Sum.addElement("scalb", 0, ReturnValue{});
  Sum.addElement("scalb", 1, ReturnValue{});

  // scalbf
  Sum.addElement("scalbf", 0, ReturnValue{});
  Sum.addElement("scalbf", 1, ReturnValue{});

  // scalbl
  Sum.addElement("scalbl", 0, ReturnValue{});
  Sum.addElement("scalbf", 1, ReturnValue{});

  // scalbln
  Sum.addElement("scalbln", 0, ReturnValue{});
  Sum.addElement("scalbln", 1, ReturnValue{});

  // scalblnf
  Sum.addElement("scalblnf", 0, ReturnValue{});
  Sum.addElement("scalblnf", 0, ReturnValue{});

  // scalblnl
  Sum.addElement("scalblnl", 0, ReturnValue{});
  Sum.addElement("scalblnl", 1, ReturnValue{});

  // scalbn
  Sum.addElement("scalbn", 0, ReturnValue{});
  Sum.addElement("scalbn", 1, ReturnValue{});

  // scalbnf
  Sum.addElement("scalbnf", 0, ReturnValue{});
  Sum.addElement("scalbnf", 1, ReturnValue{});

  // scalbnl
  Sum.addElement("scalbnl", 0, ReturnValue{});
  Sum.addElement("scalbnl", 1, ReturnValue{});

  // scandir
  Sum.addElement("scandir", 0, Parameter{});

  // secure_getenv
  Sum.addElement("secure_getenv", 0, ReturnValue{});

  // sem_getvalue
  Sum.addElement("sem_getvalue", 0, Parameter{});

  // sem_init
  Sum.addElement("sem_init", 2, Parameter{0});

  // setitimer
  Sum.addElement("setitimer", 0, Parameter{2});
  Sum.addElement("setitimer", 1, Parameter{0});

  // setlocale
  Sum.addElement("setlocale", 1, ReturnValue{});

  // setpayload
  Sum.addElement("setpayload", 1, Parameter{0});

  // setpayloadf
  Sum.addElement("setpayloadf", 1, Parameter{});

  // setpayloadl
  Sum.addElement("setpayloadl", 1, Parameter{});

  // setpayloadsig
  Sum.addElement("setpayloadsig", 1, Parameter{0});

  // setpayloadsigf
  Sum.addElement("setpayloadsigf", 1, Parameter{0});

  // setpayloadsigl
  Sum.addElement("setpayloadsigl", 1, Parameter{0});

  // setstate_r
  Sum.addElement("setstate_r", 0, Parameter{1});

  // sigaddset
  Sum.addElement("sigaddset", 1, Parameter{0});

  // signbit
  Sum.addElement("signbit", 0, ReturnValue{});

  // significand
  Sum.addElement("significand", 0, ReturnValue{});

  // significandf
  Sum.addElement("significandf", 0, ReturnValue{});

  // significandl
  Sum.addElement("significandl", 0, ReturnValue{});

  // sin
  Sum.addElement("sin", 0, ReturnValue{});

  // sincos
  Sum.addElement("sincos", 0, Parameter{1});
  Sum.addElement("sincos", 0, Parameter{2});

  // sincosf
  Sum.addElement("sincosf", 0, Parameter{1});
  Sum.addElement("sincosf", 0, Parameter{2});

  // sincosl
  Sum.addElement("sincosl", 0, Parameter{1});
  Sum.addElement("sincosl", 0, Parameter{2});

  // sinf
  Sum.addElement("sinf", 0, ReturnValue{});

  // sinl
  Sum.addElement("sinl", 0, ReturnValue{});

  // sinh
  Sum.addElement("sinh", 0, ReturnValue{});

  // sinhf
  Sum.addElement("sinhf", 0, ReturnValue{});

  // sinhl
  Sum.addElement("sinhl", 0, ReturnValue{});

  // snprintf
  Sum.addElement("snprintf", 1, Parameter{1});
  Sum.addElement("snprintf", 1, Parameter{2});
  Sum.addElement("snprintf", 1, Parameter{3});

  // sqrt
  Sum.addElement("sqrt", 0, ReturnValue{});

  // sqrtf
  Sum.addElement("sqrtf", 0, ReturnValue{});

  // sqrtl
  Sum.addElement("sqrtl", 0, ReturnValue{});

  // sscanf
  Sum.addElement("sscanf", 0, Parameter{2});
  Sum.addElement("sscanf", 1, Parameter{2});

  // stat
  Sum.addElement("stat", 0, Parameter{1});

  // stpcpy
  Sum.addElement("stpcpy", 1, Parameter{0});
  Sum.addElement("stpcpy", 0, ReturnValue{});

  // stpncpy
  Sum.addElement("stpncpy", 1, Parameter{0});
  Sum.addElement("stpncpy", 0, ReturnValue{});

  // strcat
  Sum.addElement("strcat", 1, Parameter{0});
  Sum.addElement("strcat", 0, ReturnValue{});

  // strchrnul
  Sum.addElement("strchrnul", 0, ReturnValue{});

  // strcpy
  Sum.addElement("strcpy", 1, Parameter{0});
  Sum.addElement("strcpy", 0, ReturnValue{});

  // strdup
  Sum.addElement("strdup", 0, ReturnValue{});

  // strdupa
  Sum.addElement("strdupa", 0, ReturnValue{});

  // strerror
  Sum.addElement("strerror", 0, ReturnValue{});

  // strerror_r
  Sum.addElement("strerror_r", 0, Parameter{1});
  Sum.addElement("strerror_r", 1, ReturnValue{});

  // strfromd
  Sum.addElement("strfromd", 2, Parameter{0});
  Sum.addElement("strfromd", 3, Parameter{0});

  // strfromf
  Sum.addElement("strfromf", 2, Parameter{0});
  Sum.addElement("strfromf", 3, Parameter{0});

  // strfroml
  Sum.addElement("strfroml", 2, Parameter{0});
  Sum.addElement("strfroml", 3, Parameter{0});

  // strfry
  Sum.addElement("strfry", 0, ReturnValue{});

  // strftime
  Sum.addElement("strftime", 3, Parameter{0});

  // strncat
  Sum.addElement("strncat", 1, Parameter{0});
  Sum.addElement("strncat", 0, ReturnValue{});

  // strncpy
  Sum.addElement("strncpy", 1, Parameter{0});
  Sum.addElement("strncpy", 0, ReturnValue{});

  // strndup
  Sum.addElement("strndup", 0, ReturnValue{});

  // strndupa
  Sum.addElement("strndupa", 0, ReturnValue{});

  // strpbrk
  Sum.addElement("strpbrk", 0, ReturnValue{});

  // strptime
  Sum.addElement("strptime", 0, Parameter{2});
  Sum.addElement("strptime", 0, ReturnValue{});

  // strrchr
  Sum.addElement("strrchr", 0, ReturnValue{});

  // strsep
  Sum.addElement("strsep", 0, ReturnValue{});

  // strsignal
  Sum.addElement("strsignal", 0, ReturnValue{});

  // strstr
  Sum.addElement("strstr", 0, ReturnValue{});

  // strtod
  Sum.addElement("strtod", 0, Parameter{1});
  Sum.addElement("strtod", 0, ReturnValue{});

  // strtof
  Sum.addElement("strtof", 0, Parameter{1});
  Sum.addElement("strtof", 0, ReturnValue{});

  // strtoimax
  Sum.addElement("strtoimax", 0, Parameter{1});
  Sum.addElement("strtoimax", 0, ReturnValue{});

  // strtok
  Sum.addElement("strtok", 0, ReturnValue{});

  // strtok_r
  Sum.addElement("strtok_r", 0, ReturnValue{});

  // strtol
  Sum.addElement("strtol", 0, Parameter{1});
  Sum.addElement("strtol", 0, ReturnValue{});

  // strtold
  Sum.addElement("strtold", 0, Parameter{1});
  Sum.addElement("strtold", 0, ReturnValue{});

  // strtoll
  Sum.addElement("strtoll", 0, Parameter{1});
  Sum.addElement("strtoll", 0, ReturnValue{});

  // strtoul
  Sum.addElement("strtoul", 0, Parameter{1});
  Sum.addElement("strtoul", 0, ReturnValue{});

  // strtoull
  Sum.addElement("strtoull", 0, Parameter{1});
  Sum.addElement("strtoull", 0, ReturnValue{});

  // strtoumax
  Sum.addElement("strtoumax", 0, Parameter{1});
  Sum.addElement("strtoumax", 0, ReturnValue{});

  // strxfrm
  Sum.addElement("strxfrm", 1, Parameter{});

  // swapcontext
  Sum.addElement("swapcontext", 0, Parameter{});

  // swprintf
  Sum.addElement("swprintf", 1, Parameter{0});
  Sum.addElement("swprintf", 2, Parameter{0});
  Sum.addElement("swprintf", 3, Parameter{0});

  // swscanf
  Sum.addElement("swscanf", 0, Parameter{2});
  Sum.addElement("swscanf", 1, Parameter{2});

  // symlink
  Sum.addElement("symlink", 0, Parameter{1});

  // tan
  Sum.addElement("tan", 0, ReturnValue{});

  // tanf
  Sum.addElement("tanf", 0, ReturnValue{});

  // tanh
  Sum.addElement("tanh", 0, ReturnValue{});

  // tanhf
  Sum.addElement("tanhf", 0, ReturnValue{});

  // tanhl
  Sum.addElement("tanhl", 0, ReturnValue{});

  // tanl
  Sum.addElement("tanl", 0, ReturnValue{});

  // tcgetattr
  Sum.addElement("tcgetattr", 0, Parameter{1});

  // telldir
  Sum.addElement("telldir", 0, ReturnValue{});

  // tempnam
  Sum.addElement("tempnam", 0, ReturnValue{});
  Sum.addElement("tempnam", 1, ReturnValue{});

  // tfind
  Sum.addElement("tfind", 1, ReturnValue{});

  // tgamma
  Sum.addElement("tgamma", 0, ReturnValue{});

  // tgammaf
  Sum.addElement("tgammaf", 0, ReturnValue{});

  // tgammal
  Sum.addElement("tgammal", 0, ReturnValue{});

  // timegm
  Sum.addElement("timegm", 0, ReturnValue{});

  // timelocal
  Sum.addElement("timelocal", 0, ReturnValue{});

  // tmpnam
  Sum.addElement("tmpnam", 0, ReturnValue{});

  // tmpnam_r
  Sum.addElement("tmpnam_r", 0, ReturnValue{});

  // toascii
  Sum.addElement("toascii", 0, ReturnValue{});

  // tolower
  Sum.addElement("tolower", 0, ReturnValue{});

  // toupper
  Sum.addElement("toupper", 0, ReturnValue{});

  // towctrans
  Sum.addElement("towctrans", 0, ReturnValue{});

  // towlower
  Sum.addElement("towlower", 0, ReturnValue{});

  // towupper
  Sum.addElement("towupper", 0, ReturnValue{});

  // trunc
  Sum.addElement("trunc", 0, ReturnValue{});

  // truncf
  Sum.addElement("truncf", 0, ReturnValue{});

  // truncl
  Sum.addElement("truncl", 0, ReturnValue{});

  // truncate
  Sum.addElement("truncate", 0, Parameter{0});

  // tsearch
  Sum.addElement("tsearch", 1, ReturnValue{});
  Sum.addElement("tsearch", 0, Parameter{1});

  // ufromfp
  Sum.addElement("ufromfp", 0, ReturnValue{});

  // ufromfpf
  Sum.addElement("ufromfpf", 0, ReturnValue{});

  // ufromfpl
  Sum.addElement("ufromfpl", 0, ReturnValue{});

  // ufromfpx
  Sum.addElement("ufromfpx", 0, ReturnValue{});

  // ufromfpxf
  Sum.addElement("ufromfpxf", 0, ReturnValue{});

  // ufromfpxl
  Sum.addElement("ufromfpxl", 0, ReturnValue{});

  // ungetc
  Sum.addElement("ungetc", 0, Parameter{1});

  // ungetwc
  Sum.addElement("ungetwc", 0, Parameter{1});

  // updwtmp
  Sum.addElement("updwtmp", 1, Parameter{0});

  // utime
  Sum.addElement("utime", 1, Parameter{0});

  // utimes
  Sum.addElement("utimes", 1, Parameter{0});

  // vasprintf
  Sum.addElement("vasprintf", 1, Parameter{0});
  Sum.addElement("vasprintf", 2, Parameter{0});

  // va_copy
  Sum.addElement("va_copy", 1, Parameter{0});

  // vfprintf
  Sum.addElement("vfprintf", 1, Parameter{0});
  Sum.addElement("vfprintf", 2, Parameter{0});

  // vfscanf
  Sum.addElement("vfscanf", 0, Parameter{2});

  // vfwprintf
  Sum.addElement("vfwprintf", 1, Parameter{0});
  Sum.addElement("vfwprintf", 2, Parameter{0});

  // vfwscanf
  Sum.addElement("vfwscanf", 0, Parameter{2});
  Sum.addElement("vfwscanf", 1, Parameter{2});

  // vsnprintf
  Sum.addElement("vsnprintf", 3, Parameter{0});
  Sum.addElement("vsnprintf", 2, Parameter{0});

  // vsprintf
  Sum.addElement("vsprintf", 2, Parameter{0});
  Sum.addElement("vsprintf", 1, Parameter{0});

  // vsscanf
  Sum.addElement("vsscanf", 0, Parameter{2});
  Sum.addElement("vsscanf", 1, Parameter{2});

  // vswprintf
  Sum.addElement("vswprintf", 2, Parameter{0});
  Sum.addElement("vswprintf", 1, Parameter{0});

  // vswscanf
  Sum.addElement("vswscanf", 0, Parameter{2});
  Sum.addElement("vswscanf", 1, Parameter{2});

  // wcpcpy
  Sum.addElement("wcpcpy", 1, Parameter{0});
  Sum.addElement("wcpcpy", 0, ReturnValue{});

  // wcpncpy
  Sum.addElement("wcpncpy", 1, Parameter{0});
  Sum.addElement("wcpncpy", 0, ReturnValue{});

  // wcrtomb
  Sum.addElement("wcrtomb", 1, Parameter{0});

  // wcscat
  Sum.addElement("wcscat", 1, Parameter{0});
  Sum.addElement("wcscat", 0, ReturnValue{});

  // wcschr
  Sum.addElement("wcschr", 0, ReturnValue{});

  // wcscpy
  Sum.addElement("wcscpy", 1, Parameter{0});
  Sum.addElement("wcscpy", 0, ReturnValue{});

  // wcsdup
  Sum.addElement("wcsdup", 0, ReturnValue{});

  // wcsncat
  Sum.addElement("wcsncat", 1, Parameter{0});
  Sum.addElement("wcsncat", 0, ReturnValue{});

  // wcsncpy
  Sum.addElement("wcsncpy", 1, Parameter{0});
  Sum.addElement("wcsncpy", 0, ReturnValue{});

  // wcsnrtombs
  Sum.addElement("wcsnrtombs", 1, Parameter{0});

  // wcspbrk
  Sum.addElement("wcspbrk", 0, ReturnValue{});

  // wcsrchr
  Sum.addElement("wcsrchr", 0, ReturnValue{});

  // wcsrtombs
  Sum.addElement("wcsrtombs", 1, Parameter{0});

  // wcsstr
  Sum.addElement("wcsstr", 0, ReturnValue{});

  // wcstod
  Sum.addElement("wcstod", 0, Parameter{1});
  Sum.addElement("wcstod", 0, ReturnValue{});

  // wcstof
  Sum.addElement("wcstof", 0, Parameter{1});
  Sum.addElement("wcstof", 0, ReturnValue{});

  // wcstoimax
  Sum.addElement("wcstoimax", 0, Parameter{0});
  Sum.addElement("wcstoimax", 0, ReturnValue{});

  // wcstok
  Sum.addElement("wcstok", 0, ReturnValue{});
  Sum.addElement("wcstok", 2, ReturnValue{});

  // wcstol
  Sum.addElement("wcstol", 0, Parameter{1});
  Sum.addElement("wcstol", 0, ReturnValue{});

  // wcstold
  Sum.addElement("wcstold", 0, Parameter{1});
  Sum.addElement("wcstold", 0, ReturnValue{});

  // wcstoll
  Sum.addElement("wcstoll", 0, Parameter{1});
  Sum.addElement("wcstoll", 0, ReturnValue{});

  // wcstombs
  Sum.addElement("wcstombs", 1, Parameter{0});

  // wcstoul
  Sum.addElement("wcstoul", 0, Parameter{1});
  Sum.addElement("wcstoul", 0, ReturnValue{});

  // wcstoull
  Sum.addElement("wcstoull", 0, Parameter{1});
  Sum.addElement("wcstoull", 0, ReturnValue{});

  // wcstoumax
  Sum.addElement("wcstoumax", 0, Parameter{1});
  Sum.addElement("wcstoumax", 0, ReturnValue{});

  // wcsxfrm
  Sum.addElement("wcsxfrm", 1, Parameter{0});

  // wctob
  Sum.addElement("wctob", 0, ReturnValue{});

  // wctomb
  Sum.addElement("wctomb", 1, Parameter{0});

  // wctrans
  Sum.addElement("wctrans", 0, ReturnValue{});

  // wctype
  Sum.addElement("wctype", 0, ReturnValue{});

  // wmemchr
  Sum.addElement("wmemchr", 0, ReturnValue{});

  // wmemcpy
  Sum.addElement("wmemcpy", 1, Parameter{0});
  Sum.addElement("wmemcpy", 0, ReturnValue{});

  // wmemmove
  Sum.addElement("wmemmove", 1, Parameter{0});
  Sum.addElement("wmemmove", 0, ReturnValue{});

  // wmempcpy
  Sum.addElement("wmempcpy", 1, Parameter{0});
  Sum.addElement("wmempcpy", 0, ReturnValue{});

  // wmemset
  Sum.addElement("wmemset", 1, Parameter{0});
  Sum.addElement("wmemset", 0, ReturnValue{});

  // wordexp
  Sum.addElement("wordexp", 0, Parameter{1});

  // y0
  Sum.addElement("y0", 0, ReturnValue{});

  // y0f
  Sum.addElement("y0f", 0, ReturnValue{});

  // y0l
  Sum.addElement("y0l", 0, ReturnValue{});

  // y1
  Sum.addElement("y1", 0, ReturnValue{});

  // y1f
  Sum.addElement("y1f", 0, ReturnValue{});

  // y1l
  Sum.addElement("y1l", 0, ReturnValue{});

  // yn
  Sum.addElement("yn", 0, ReturnValue{});
  Sum.addElement("yn", 1, ReturnValue{});

  // ynf
  Sum.addElement("ynf", 0, ReturnValue{});
  Sum.addElement("ynf", 1, ReturnValue{});

  // ynl
  Sum.addElement("ynl", 0, ReturnValue{});
  Sum.addElement("ynl", 1, ReturnValue{});

  return Sum;
}

const library_summary::FunctionDataFlowFacts &psr::getLibCSummary() {
  static const auto Sum = createLibCSummary();
  return Sum;
}
