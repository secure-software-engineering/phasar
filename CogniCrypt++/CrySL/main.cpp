#include <CrySLParserEngine.h>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>

// This is a simple entrypoint for a demo program showing, that the CrySL parser
// is working

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Please specify an input-directory" << std::endl;
    return 1;
  }

  std::vector<std::string> crysl_filenames;
  for (auto &entry : std::filesystem::directory_iterator(argv[1]) ){
    if (entry.is_regular_file()) {
      auto ext = entry.path().extension();
      if (ext == ".cryptosl" || ext == ".cryptsl" || ext == ".crysl") {
        crysl_filenames.push_back(entry.path().string());
      }
    }
  }
  
  CCPP::CrySLParserEngine engine(std::move(crysl_filenames));

  if (engine.parseAndTypecheck()) {
    std::cout << "Parsing and typechecking successful" << std::endl;
    return 0;
  } else {
    std::cerr << "Finished with errors" << std::endl;
    return 1;
  }
  
}