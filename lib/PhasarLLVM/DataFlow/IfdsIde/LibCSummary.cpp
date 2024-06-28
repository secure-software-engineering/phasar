#include "phasar/PhasarLLVM/DataFlow/IfdsIde/LibCSummary.h"

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/FunctionDataFlowFacts.h"

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

  

  // TODO
  return Sum;
}

const FunctionDataFlowFacts &psr::getLibCSummary() {
  static const auto Sum = createLibCSummary();
  return Sum;
}
