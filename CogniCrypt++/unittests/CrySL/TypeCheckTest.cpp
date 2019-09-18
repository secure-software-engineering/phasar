// Unit Test suite
#include <../../utils/Logger.h>
#include <Parser/CrySLParserEngine.h>
#include <filesystem>
#include <gtest/gtest.h>
#include <iostream>

using namespace CCPP;

/* ============== TEST FIXTURE ============== */

class TypeCheckTest : public ::testing::Test {

protected:
  const std::string pathToCrySLFiles = "../../../test/CrySL/";
  const std::vector<std::string> EntryPoints = {"main"};

  CrySLParserEngine *cryslparserengine;

  TypeCheckTest() {}
  virtual ~TypeCheckTest() {}

  void Initialize(const std::vector<std::string> &CrySLFiles) {
    // std::cout << "Working directory: " << std::filesystem::current_path()
    //          << std::endl;
    for (const auto &file : CrySLFiles) {
      EXPECT_EQ(std::filesystem::exists(file), true) << file;
    }
    cryslparserengine = new CrySLParserEngine(CrySLFiles);
    cryslparserengine->parseAndTypecheck();
  }

  void SetUp() override { bl::core::get()->set_logging_enabled(false); }

  void TearDown() override {
    if (cryslparserengine) {
      delete cryslparserengine;
      cryslparserengine = nullptr;
    }
  }

  void compareResults(bool groundTruth, size_t groundValue) {
    ASSERT_EQ(cryslparserengine == nullptr, false)
        << "CrySLParserEngine is null";
    size_t numErrors = cryslparserengine->getNumberOfSyntaxErrors();
    bool typeCheckResult = cryslparserengine->typecheckSucceeded();
    

    EXPECT_EQ(numErrors, groundValue);
    EXPECT_EQ(typeCheckResult, groundTruth);
  }

}; // class Fixture

TEST_F(TypeCheckTest, TypeCheck_Cipher_CTX) {
  Initialize({pathToCrySLFiles + "EVP_CIPHER_CTX.cryptosl"});
  bool groundTruth = true;
  size_t groundValue = 0;
  compareResults(groundTruth, groundValue);
}

TEST_F(TypeCheckTest, TypeCheck_Cipher) {
  Initialize({pathToCrySLFiles + "EVP_CIPHER.cryptosl"});
  bool groundTruth = true;
  size_t groundValue = 0;
  compareResults(groundTruth, groundValue);
}

TEST_F(TypeCheckTest, TypeCheck_MD_CTX) {
  Initialize({pathToCrySLFiles + "EVP_MD.cryptosl"});
  bool groundTruth = true;
  size_t groundValue = 0;
  compareResults(groundTruth, groundValue);
}

TEST_F(TypeCheckTest, TypeCheck_MD_CTX_incorrect) {
  Initialize({pathToCrySLFiles + "EVP_MD_incorrect.cryptosl"});
  bool groundTruth = false;
  size_t groundValue = 0;
  compareResults(groundTruth, groundValue);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
