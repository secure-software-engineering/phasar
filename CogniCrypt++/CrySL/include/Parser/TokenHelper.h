#include <string>
namespace CCPP {
/// \brief Parses the int-literal intVal using std::stoll(std::string&)
long long parseInt(const std::string &intVal);
/// \brief Parses the string-literal stringVal
///
/// Removes the double-quotes at the beginning and the end and resolves all
/// string-escapes (without Hex and Unicode escapes)
const std::string parseString(const std::string &stringVal);
/// \brief Parses the floating-point literal floatVal using
/// std::stod(std::string&)
double parseFloat(const std::string &floatVal);
/// \brief Parses the character literal charVal
///
/// Removes the single-quotes at the beginning and end and Resolves the
/// char-escapes (without Hex and Unicode escapes)
char parseChar(const std::string &charVal);
/// \brief Parses the boolean literal boolVal into true or false
bool parseBool(const std::string &boolVal);
} // namespace CCPP