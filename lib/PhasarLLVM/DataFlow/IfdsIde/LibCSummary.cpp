#include "phasar/PhasarLLVM/DataFlow/IfdsIde/LibCSummary.h"

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/FunctionDataFlowFacts.h"
#include "clang/AST/Attrs.inc"

using namespace psr;

static FunctionDataFlowFacts createLibCSummary() {
  FunctionDataFlowFacts Sum;

  // TODO: Use public API instead!
  Sum.fdff["localtime_r"][1].emplace_back(ReturnValue{});

  // Sum.fdff["foo"][2].emplace_back(Parameter{1});
  //abs
  Sum.addElement("abs", 0, ReturnValue{});

  //acos
  Sum.addElement("acos", 0, ReturnValue{});

  //acosf
  Sum.addElement("acosf", 0, ReturnValue{});

  //acosh
  Sum.addElement("acosh", 0, ReturnValue{});

  //acoshf
  Sum.addElement("acoshf", 0, ReturnValue{});

  //acoshl
  Sum.addElement("acoshl", 0, ReturnValue{});

  //acosl
  Sum.addElement("acosl", 0, ReturnValue{});

  //argz_add
  Sum.addElement("argz_add", 2, Parameter{0});

  //argz_add_sep
  Sum.addElement("argz_add_sep", 2, Parameter{0});

  //argz_append
  Sum.addElement("argz_append", 2, Parameter{0});
  Sum.addElement("argz_append", 3, Parameter{1});

  //agrz_create
  Sum.addElement("argz_create", 0, Parameter{1});

  //argz_create_sep
  Sum.addElement("argz_create_sep", 0, Parameter{2});

  //argz_extract
  Sum.addElement("argz_extract", 0, Parameter{2});

  //argz_insert
  Sum.addElement("argz_insert", 3, Parameter{0});

  //argz_next
  Sum.addElement("argz_next", 0, ReturnValue{});

  //argz_replace
  Sum.addElement("argz_replace", 0, Parameter{0});

  //argz_stringify
  Sum.addElement("argz_stringify", 2, Parameter{0});

  //asin
  Sum.addElement("asin", 0, ReturnValue{});

  //asinf
  Sum.addElement("asinf", 0, ReturnValue{});

  //asinh
  Sum.addElement("asinh", 0, ReturnValue{});

  //asinhf
  Sum.addElement("asinhf", 0, ReturnValue{});

  //asinhl
  Sum.addElement("asinhl", 0, ReturnValue{});

  //asinl
  Sum.addElement("asinl", 0, ReturnValue{});

  //asprintf
  Sum.addElement("asprintf", 1, Parameter{0});
  Sum.addElement("asprintf", 2, Parameter{0});
  Sum.addElement("asprintf", 3, Parameter{0});
  Sum.addElement("asprintf", 4, Parameter{0});
  Sum.addElement("asprintf", 5, Parameter{0});

  //atan
  Sum.addElement("atan", 0, ReturnValue{});

  //atan2
  Sum.addElement("atan2", 0, ReturnValue{});
  Sum.addElement("atan2", 1, ReturnValue{});

  //atan2f
  Sum.addElement("atan2f", 0, ReturnValue{});
  Sum.addElement("atan2f", 1, ReturnValue{});

  //atan2l
  Sum.addElement("atan2l", 0, ReturnValue{});
  Sum.addElement("atan2l", 1, ReturnValue{});

  //atanf
  Sum.addElement("atanf", 0, ReturnValue{});

  //atanh
  Sum.addElement("atanh", 0, ReturnValue{});

  //atanhf
  Sum.addElement("atanhf", 0, ReturnValue{});

  //atanhl
  Sum.addElement("atanhl", 0, ReturnValue{});

  //atanl
  Sum.addElement("atanl", 0, ReturnValue{});

  //basename
  Sum.addElement("basename", 0, ReturnValue{});

  //bcopy
  Sum.addElement("bcopy", 0, Parameter{1});

  //bindtextdomain
  Sum.addElement("bindtextdomain", 1, ReturnValue{});

  //bind_textdomain_codeset
  Sum.addElement("bind_textdomain_codeset", 1, ReturnValue{});

  //bsearch
  Sum.addElement("bsearch", 1, ReturnValue{});

  //btowc
  Sum.addElement("btowc", 0, ReturnValue{});

  //cabs
  Sum.addElement("cabs", 0, ReturnValue{});

  //cabsf
  Sum.addElement("cabsf", 0, ReturnValue{});

  //cabsl
  Sum.addElement("cabsl", 0, ReturnValue{});

  //cacos
  Sum.addElement("cacos", 0, ReturnValue{});

  //cacosf
  Sum.addElement("cacosf", 0, ReturnValue{});

  //cacosl
  Sum.addElement("cacosl", 0, ReturnValue{});

  //cacosh
  Sum.addElement("cacosh", 0, ReturnValue{});

  //cacoshf
  Sum.addElement("cacoshf", 0, ReturnValue{});

  //cacoshl
  Sum.addElement("cacoshl", 0, ReturnValue{});

  //carg
  Sum.addElement("carg", 0, ReturnValue{});

  //cargf
  Sum.addElement("cargf", 0, ReturnValue{});

  //cargl
  Sum.addElement("cargl", 0, ReturnValue{});

  //casin
  Sum.addElement("casin", 0, ReturnValue{});

  //casinf
  Sum.addElement("casinf", 0, ReturnValue{});

  //casinh
  Sum.addElement("casinh", 0, ReturnValue{});

  //casinhf
  Sum.addElement("casinhf", 0, ReturnValue{});

  //casinhl
  Sum.addElement("casinhl", 0, ReturnValue{});

  //casinl
  Sum.addElement("casinl", 0, ReturnValue{});

  //catan
  Sum.addElement("catan", 0, ReturnValue{});

  //catanf
  Sum.addElement("catanf", 0, ReturnValue{});

  //catanh
  Sum.addElement("catanh", 0, ReturnValue{});

  //catanhf
  Sum.addElement("catanhf", 0, ReturnValue{});

  //catanhl
  Sum.addElement("catanhl", 0, ReturnValue{});

  //catanl
  Sum.addElement("catanl", 0, ReturnValue{});

  //catgets
  Sum.addElement("catgets", 3, ReturnValue{});

  //cbrt
  Sum.addElement("cbrt", 0, ReturnValue{});

  //cbrtf
  Sum.addElement("cbrtf", 0, ReturnValue{});

  //cbrtl
  Sum.addElement("cbrtl", 0, ReturnValue{});

  //ccos
  Sum.addElement("ccos", 0, ReturnValue{});

  //ccosf
  Sum.addElement("ccosf", 0, ReturnValue{});

  //ccosh
  Sum.addElement("ccosh", 0, ReturnValue{});

  //ccoshf
  Sum.addElement("ccoshf", 0, ReturnValue{});

  //ccoshl
  Sum.addElement("ccoshl", 0, ReturnValue{});

  //ccosl
  Sum.addElement("ccosl", 0, ReturnValue{});

  //ceil
  Sum.addElement("ceil", 0, ReturnValue{});

  //ceilf
  Sum.addElement("ceilf", 0, ReturnValue{});

  //ceill
  Sum.addElement("ceill", 0, ReturnValue{});

  //cexp
  Sum.addElement("cexp", 0, ReturnValue{});

  //cexpf
  Sum.addElement("cexpf", 0, ReturnValue{});

  //cexpl
  Sum.addElement("cexpl", 0, ReturnValue{});

  //cfgetispeed
  Sum.addElement("cfgetispeed", 0, ReturnValue{});

  //cfgetospeed
  Sum.addElement("cfgetospeed", 0, ReturnValue{});

  //cimag
  Sum.addElement("cimag", 0, ReturnValue{});

  //cimagf
  Sum.addElement("cimagf", 0, ReturnValue{});

  //cimagl
  Sum.addElement("cimagl", 0, ReturnValue{});

  //clog
  Sum.addElement("clog", 0, ReturnValue{});

  //clog10
  Sum.addElement("clog10", 0, ReturnValue{});

  //clog10f
  Sum.addElement("clog10f", 0, ReturnValue{});

  //clog10l
  Sum.addElement("clog10l",0, ReturnValue{});

  //clogf
  Sum.addElement("clogf", 0, ReturnValue{});

  //clogl
  Sum.addElement("clogl", 0, ReturnValue{});

  //conj
  Sum.addElement("conj", 0, ReturnValue{});

  //conjf
  Sum.addElement("conjf", 0, ReturnValue{});

  //conjl
  Sum.addElement("conjl", 0, ReturnValue{});

  //copysign
  Sum.addElement("copysign", 0, ReturnValue{});
  Sum.addElement("copysign", 1, ReturnValue{});

  //copysignf
  Sum.addElement("copysignf", 0, ReturnValue{});
  Sum.addElement("copysign", 1, ReturnValue{});

  //copysignl
  Sum.addElement("copysignl", 0, ReturnValue{});
  Sum.addElement("copysignl", 1, ReturnValue{});

  //cos
  Sum.addElement("cos", 0, ReturnValue{});

  //cosf
  Sum.addElement("cosf", 0, ReturnValue{});

  //cosh
  Sum.addElement("cosh", 0, ReturnValue{});

  //coshf
  Sum.addElement("coshf", 0, ReturnValue{});

  //coshl
  Sum.addElement("coshl", 0, ReturnValue{});

  //cosl
  Sum.addElement("cosl", 0, ReturnValue{});

  //cpow
  Sum.addElement("cpow", 0, ReturnValue{});
  Sum.addElement("cpow", 1, ReturnValue{});

  //cpowf
  Sum.addElement("cpowf", 0, ReturnValue{});
  Sum.addElement("cpowf", 1, ReturnValue{});

  //cpowl
  Sum.addElement("cpowl", 0, ReturnValue{});
  Sum.addElement("cpowl", 1, ReturnValue{});

  //cproj
  Sum.addElement("cproj", 0, ReturnValue{});

  //cprojf
  Sum.addElement("cproj", 0, ReturnValue{});

  //cprojl
  Sum.addElement("cprojl", 0, ReturnValue{});

  //creal
  Sum.addElement("creal", 0, ReturnValue{});

  //crealf
  Sum.addElement("crealf", 0, ReturnValue{});

  //creall
  Sum.addElement("creall", 0, ReturnValue{});

  //crypt
  Sum.addElement("crypt", 0, ReturnValue{});
  //Sum.addElement("crypt", 1, ReturnValue{});

  //crypt_r
  Sum.addElement("crypt_r", 0, ReturnValue{});
  //Sum.addElement("crypt_r", 1, ReturnValue{});

  //csin
  Sum.addElement("csin", 0, ReturnValue{});

  //csinf
  Sum.addElement("csinf", 0, ReturnValue{});

  //csinh
  Sum.addElement("csinh", 0, ReturnValue{});

  //csinhf
  Sum.addElement("csinhf", 0, ReturnValue{});

  //csinhl
  Sum.addElement("csinhl", 0, ReturnValue{});

  //csinl
  Sum.addElement("csinl", 0, ReturnValue{});

  //csprt
  Sum.addElement("csqrt", 0, ReturnValue{});

  //csqrtf
  Sum.addElement("csqrtf", 0, ReturnValue{});

  //csqrtl
  Sum.addElement("csqrtl", 0, ReturnValue{});

  //ctan
  Sum.addElement("ctan", 0, ReturnValue{});

  //ctanf
  Sum.addElement("ctanf", 0, ReturnValue{});

  //ctanh
  Sum.addElement("ctanh", 0, ReturnValue{});

  //ctanhf
  Sum.addElement("ctanhf", 0, ReturnValue{});

  //ctanhl
  Sum.addElement("ctanhl", 0, ReturnValue{});

  //ctanl
  Sum.addElement("ctanl", 0, ReturnValue{});

  //ctermid
  Sum.addElement("ctermid", 0, ReturnValue{});

  //ctime
  Sum.addElement("ctime", 0, ReturnValue{});  //?

  //ctime_r
  Sum.addElement("ctime_r", 0, Parameter{1});

  //cuserid
  Sum.addElement("cuserid", 0, ReturnValue{});

  //dcgettext
  Sum.addElement("dcgettext", 1, ReturnValue{});

  //dcngettext
  Sum.addElement("dcngettext", 1, ReturnValue{});

  //dgettext
  Sum.addElement("dgettext", 1, ReturnValue{});

  //difftime
  Sum.addElement("difftime", 0, ReturnValue{});
  Sum.addElement("difftime", 1, ReturnValue{});

  //dirname
  Sum.addElement("dirname", 0, ReturnValue{});

  //div
  Sum.addElement("div", 0, ReturnValue{});
  Sum.addElement("div", 1, ReturnValue{});

  //dngettext
  Sum.addElement("dngettext", 1, ReturnValue{});

  //drem
  Sum.addElement("drem", 0, ReturnValue{});
  Sum.addElement("drem", 1, ReturnValue{});

  //dremf
  Sum.addElement("dremf", 0, ReturnValue{});
  Sum.addElement("dremf", 1, ReturnValue{});

  //dreml
  Sum.addElement("dreml", 0, ReturnValue{});
  Sum.addElement("dreml", 1, ReturnValue{});

  //dup
  Sum.addElement("dup", 0, ReturnValue{});

  //dup2
  Sum.addElement("dup2", 0, ReturnValue{});

  //envz_add
  Sum.addElement("envz_add", 2, Parameter{0});
  Sum.addElement("envz_add", 3, Parameter{0});

  //envz_entry
  Sum.addElement("envz_entry", 0, ReturnValue{});

  //envz_get
  Sum.addElement("envz_get", 0, ReturnValue{});

  //envz_merge
  Sum.addElement("envz_merge", 2, ReturnValue{});

  //erf
  Sum.addElement("erf", 0, ReturnValue{});

  //erfc
  Sum.addElement("erfc", 0, ReturnValue{});

  //erfcf
  Sum.addElement("erfcf", 0, ReturnValue{});

  //erfcl
  Sum.addElement("erfcf", 0, ReturnValue{});

  //erff
  Sum.addElement("erff", 0, ReturnValue{});

  //erfl
  Sum.addElement("erfl", 0, ReturnValue{});

  //exp
  Sum.addElement("exp", 0, ReturnValue{});

  //exp10
  Sum.addElement("exp10", 0, ReturnValue{});

  //exp10f
  Sum.addElement("exp10f", 0, ReturnValue{});

  //exp10l
  Sum.addElement("exp10l", 0, ReturnValue{});

  //exp2
  Sum.addElement("exp2", 0, ReturnValue{});

  //exp2f
  Sum.addElement("exp2f", 0, ReturnValue{});

  //exp2l
  Sum.addElement("exp2l", 0, ReturnValue{});

  //expf
  Sum.addElement("expf", 0, ReturnValue{});

  //expl
  Sum.addElement("expl", 0, ReturnValue{});

  //expm1
  Sum.addElement("expm1", 0, ReturnValue{});

  //expm1f
  Sum.addElement("expm1f", 0, ReturnValue{});

  //expm1l
  Sum.addElement("expm1l", 0, ReturnValue{});

  //fabs
  Sum.addElement("fabs", 0, ReturnValue{});

  //fabsf
  Sum.addElement("fabsf", 0, ReturnValue{});

  //fabsl
  Sum.addElement("fabsl", 0, ReturnValue{});

  //fdim
  Sum.addElement("fdim", 0, ReturnValue{});

  //fdimf
  Sum.addElement("fdimf", 0, ReturnValue{});

  //fdiml
  Sum.addElement("fdiml", 0, ReturnValue{});

  //fgetc
  Sum.addElement("fgetc", 0, ReturnValue{});

  //fgetpwent
  Sum.addElement("fgetpwent", 0, ReturnValue{});

  //fgetpwent_f
  Sum.addElement("fgetpwent_r", 0, Parameter{1});

  //fgets
  Sum.addElement("fgets", 2, Parameter{0});
  Sum.addElement("fgets", 0, ReturnValue{});

  //fgetwc
  Sum.addElement("fgetwc", 0, ReturnValue{});

  //fgetws
  Sum.addElement("fgetws", 2, Parameter{0});
  Sum.addElement("fgetws", 0, ReturnValue{});

  //finite
  Sum.addElement("finite", 0, ReturnValue{});

  //finitef
  Sum.addElement("finitef", 0, ReturnValue{});

  //finitel
  Sum.addElement("finitel", 0, ReturnValue{});

  //floor
  Sum.addElement("floor", 0, ReturnValue{});

  //floorf
  Sum.addElement("floorf", 0, ReturnValue{});

  //floorl
  Sum.addElement("floorl", 0, ReturnValue{});

  //fma
  Sum.addElement("fma", 0, ReturnValue{});
  Sum.addElement("fma", 1, ReturnValue{});
  Sum.addElement("fma", 2, ReturnValue{});

  //fmaf
  Sum.addElement("fmaf", 0, ReturnValue{});
  Sum.addElement("fmaf", 1, ReturnValue{});
  Sum.addElement("fmaf", 2, ReturnValue{});

  //fmal
  Sum.addElement("fmal", 0, ReturnValue{});
  Sum.addElement("fmal", 1, ReturnValue{});
  Sum.addElement("fmal", 2, ReturnValue{});

  //fmax
  Sum.addElement("fmax", 0, ReturnValue{});
  Sum.addElement("fmax", 1, ReturnValue{});

  //fmaxf
  Sum.addElement("fmaxf", 0, ReturnValue{});
  Sum.addElement("fmaxf", 1, ReturnValue{});

  //fmaxl
  Sum.addElement("fmaxl", 0, ReturnValue{});
  Sum.addElement("fmaxl", 1, ReturnValue{});

  //fmaxmag
  Sum.addElement("fmaxmag", 0, ReturnValue{});
  Sum.addElement("fmaxmag", 1, ReturnValue{});

  //fmaxmagf
  Sum.addElement("fmaxmag", 0, ReturnValue{});
  Sum.addElement("fmaxmagf", 1, ReturnValue{});

  //fmaxmagl
  Sum.addElement("fmaxmagl", 0, ReturnValue{});
  Sum.addElement("fmaxmag", 1, ReturnValue{});

  //fmin
  Sum.addElement("fmin", 0, ReturnValue{});
  Sum.addElement("fmin", 1, ReturnValue{});

  //fminf
  Sum.addElement("fminf", 0, ReturnValue{});
  Sum.addElement("fminf", 1, ReturnValue{});

  //fminl
  Sum.addElement("fminl", 0, ReturnValue{});
  Sum.addElement("fminl", 1, ReturnValue{});

  //fminmag
  Sum.addElement("fminmag", 0, ReturnValue{});
  Sum.addElement("fminmag", 1, ReturnValue{});

  //fminmagf
  Sum.addElement("fminmagf", 0, ReturnValue{});
  Sum.addElement("fminmagf", 1, ReturnValue{});

  //fminmagl
  Sum.addElement("fminmagl", 0, ReturnValue{});
  Sum.addElement("fminmagl", 1, ReturnValue{});

  //fmod
  Sum.addElement("fmod", 0, ReturnValue{});
  Sum.addElement("fmod", 1, ReturnValue{});

  //fmodf
  Sum.addElement("fmodf", 0, ReturnValue{});
  Sum.addElement("fmodf", 1, ReturnValue{});

  //fmodl
  Sum.addElement("fmodl", 0, ReturnValue{});
  Sum.addElement("fmodl", 1, ReturnValue{});

  //fprintf
  Sum.addElement("fprintf", 1, Parameter{0});
  Sum.addElement("fprintf", 2, Parameter{0});
  Sum.addElement("fprintf", 3, Parameter{0});
  //....

  //fputc
  Sum.addElement("fputc", 0, Parameter{1});

  //fputs
  Sum.addElement("fputs", 0, Parameter{1});

  //fputwc
  Sum.addElement("fputwc", 0, Parameter{1});

  //fputws
  Sum.addElement("fputws", 0, Parameter{1});

  //fread
  Sum.addElement("fread", 3, Parameter{0});

  //frexp
  Sum.addElement("frexp", 0, Parameter{1});
  Sum.addElement("frexp", 0, ReturnValue{});

  //frexpf
  Sum.addElement("frexpf", 0, Parameter{1});
  Sum.addElement("frexpf", 0, ReturnValue{});

  //frexpl
  Sum.addElement("frexpl", 0, Parameter{1});
  Sum.addElement("frexpl", 0, ReturnValue{});

  //fromfp
  Sum.addElement("fromfp", 0, ReturnValue{});

  //fromfpf
  Sum.addElement("fromfpf", 0, ReturnValue{});

  //fromfpl
  Sum.addElement("fromfpl", 0, ReturnValue{});

  //fromfpx
  Sum.addElement("fromfpx", 0, ReturnValue{});

  //fromfpxf
  Sum.addElement("fromfpxf", 0, ReturnValue{});

  //fromfpxl
  Sum.addElement("fromfpxl", 0, ReturnValue{});

  //fscanf
  Sum.addElement("fscanf", 0, Parameter{2});

  //fstat
  Sum.addElement("fstat", 0, Parameter{1});

  //fstat64
  Sum.addElement("fstat64", 0, Parameter{});

  //fwprintf
  Sum.addElement("fwprintf", 1, Parameter{0});
  Sum.addElement("fwprintf", 2, Parameter{0});
  Sum.addElement("fwprintf", 3, Parameter{0});

  //fwrite
  Sum.addElement("fwrite", 0, Parameter{3});

  //fwscanf
  Sum.addElement("fwscanf", 0, Parameter{2});

  //gamma
  Sum.addElement("gamma", 0, ReturnValue{});

  //gammaf
  Sum.addElement("gammaf", 0, ReturnValue{});

  //gammal
  Sum.addElement("gammal", 0, ReturnValue{});

  //gcvt
  Sum.addElement("gcvt", 0, Parameter{2});
  Sum.addElement("gcvt", 2, ReturnValue{});

  //getauxval
  Sum.addElement("getauxval", 0, ReturnValue{});

  //getc
  Sum.addElement("getc", 0, ReturnValue{});

  //getc_unlocked
  Sum.addElement("getc_unlocked", 0, ReturnValue{});

  //getchar
  Sum.addElement("getchar", 0, ReturnValue{});

  //getchar_unlocked
  Sum.addElement("getchar_unlocked", 0, ReturnValue{});

  //getcwd
  Sum.addElement("getcwd", 0, ReturnValue{});

  //getdate
  Sum.addElement("getdate", 0, ReturnValue{});

  //getdate_r
  Sum.addElement("getdate_r", 0, Parameter{1});

  //getdelim
  Sum.addElement("getdelim", 3, Parameter{0});

  //getline
  Sum.addElement("getline", 2, Parameter{0});

  //getpayload
  Sum.addElement("getpayoad", 0, Return{});

  //getpayloadf
  Sum.addElement("getpayloadf", 0, ReturnValue{});

  //getpayloadl
  Sum.addElement("getpayloadl", 0, ReturnValue{});

  //getpeername
  Sum.addElement("getpeername", 0, Parameter{});

  //getrlimit
  Sum.addElement("getrlimit", 1, ReturnValue{});

  //gets
  Sum.addElement("gets", 0, ReturnValue{});

  //gettext
  Sum.addElement("gettext", 0, ReturnValue{});

  //gettimeofday
  Sum.addElement("gettimeofday", 0, Parameter{1});

  //getutent_r
  Sum.addElement("getutent_r", 0, Parameter{1});

  //getutid
  Sum.addElement("getutid", 0, ReturnValue{});

  //getutid_r
  Sum.addElement("getutid", 0, Parameter{1});
  Sum.addElement("getutid", 1, Parameter{2});

  //getutline
  Sum.addElement("getutline", 0, ReturnValue{});

  



  // TODO
  return Sum;
}

const FunctionDataFlowFacts &psr::getLibCSummary() {
  static const auto Sum = createLibCSummary();
  return Sum;
}
