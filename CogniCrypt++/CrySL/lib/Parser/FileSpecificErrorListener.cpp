#include <Parser/FileSpecificErrorListener.h>
#include <iostream>
namespace CCPP {
FileSpecificErrorListener::FileSpecificErrorListener(
    const std::string &filename)
    : filename(filename) {}
void FileSpecificErrorListener::syntaxError(
    antlr4::Recognizer *recognizer, antlr4::Token *offendingSymbol, size_t line,
    size_t charPositionInLine, const std::string &msg, std::exception_ptr e) {
  std::cerr << "Error: " << filename << " at (" << line << ", "
            << charPositionInLine << "): " << msg << std::endl;
}
} // namespace CCPP