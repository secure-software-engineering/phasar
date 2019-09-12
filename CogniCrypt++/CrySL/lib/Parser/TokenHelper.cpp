#include <Parser/TokenHelper.h>

#include <sstream>
#include <string>
#include <string_view>
namespace CCPP {
using namespace std;
long long parseInt(const string &intVal) { return stoll(intVal); }
const string parseString(const string &stringVal) {
  string_view strLit = string_view(stringVal).substr(1, stringVal.size() - 2);
  stringstream ss;

  bool escape = false;
  for (size_t i = 0; i < strLit.size(); ++i) {
    if (!escape) {
      if (strLit[i] == '\\')
        escape = true;
      else
        ss.put(strLit[i]);
    } else {
      escape = false;
      switch (strLit[i]) {
      case '\\':
        ss.put('\\');
        break;
      case '"':
        ss.put('"');
        break;
      case 'n':
        ss.put('\n');
        break;
      case 'r':
        ss.put('\r');
        break;
      case 'a':
        ss.put('\a');
        break;
      case 'b':
        ss.put('\b');
        break;
      case 'f':
        ss.put('\f');
        break;
      case 't':
        ss.put('\t');
        break;
      case 'v':
        ss.put('\v');
        break;
      default:
        ss.put(strLit[i]);
        break;
      }
    }
  }
  if (escape) {
    // TODO error: unclosed escape sequence
  }
  return ss.str();
}
double parseFloat(const string &floatVal) { return stod(floatVal); }
char parseChar(const string &charVal) {
  auto charLit = string_view(charVal).substr(1, charVal.size() - 2);
  if (charLit.size() == 1) {
    return charLit[0];
  }
  switch (charLit[1]) {
  case '\\':
    return '\\';
  case '\'':
    return '\'';
  case 'n':
    return '\n';
  case 'r':
    return '\r';
  case 'a':
    return '\a';
  case 'b':
    return '\b';
  case 'f':
    return '\f';
  case 't':
    return '\t';
  case 'v':
    return '\v';
  default:
    return charLit[1];
  }
}
bool parseBool(const string &boolVal) {
  if (boolVal == "true")
    return true;
  return false;
}
} // namespace CCPP