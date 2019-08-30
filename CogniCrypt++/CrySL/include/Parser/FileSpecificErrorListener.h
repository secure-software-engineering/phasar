#include <antlr4-runtime.h>
#include <string>
namespace CCPP {
class FileSpecificErrorListener : public antlr4::BaseErrorListener {
  const std::string &filename;

public:
  FileSpecificErrorListener(const std::string &filename);
  virtual void syntaxError(antlr4::Recognizer *recognizer,
                           antlr4::Token *offendingSymbol, size_t line,
                           size_t charPositionInLine, const std::string &msg,
                           std::exception_ptr e) override;
};
} // namespace CCPP