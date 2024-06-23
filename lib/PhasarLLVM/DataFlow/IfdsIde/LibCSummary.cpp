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
  // TODO
  return Sum;
}

const FunctionDataFlowFacts &psr::getLibCSummary() {
  static const auto Sum = createLibCSummary();
  return Sum;
}
