// Unit Test suite
#include <Parser/CrySLParserEngine.h>
#include <gtest/gtest.h>
#include <iostream>

using namespace CCPP;

/* ============== TEST FIXTURE ============== */

class TypeCheckTest : public ::testing::Test {

protected:
  const std::string pathToCrySLFiles =
      PhasarConfig::getPhasarConfig().PhasarDirectory() +
      "build/test/CogniCrypt++_tests/";
  const std::vector<std::string> EntryPoints = {"main"};

  CrySLParserEngine *cryslparserengine;

  TypeCheckTest() {}
  virtual ~TypeCheckTest() {}

  void Initialize(const std::vector<std::string> &CrySLFiles) {

    cryslparserengine = new CrySLParserEngine(CrySLFiles);
  }

  void SetUp() override { bl::core::get()->set_logging_enabled(false); }

  void TearDown() override { delete cryslparserengine; }

  void compareResults(bool groundTruth, size_t groundValue) {

    size_t numErrors = cryslparserengine.getNumberOfSyntaxErrors();
    bool typeCheckResult = cryslparserengine.typecheckSucceeded();

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
  Initialize({pathToCrySLFiles + "EVP_MC_CTX.cryptosl"});
  bool groundTruth = true;
  size_t groundValue = 0;
  compareResults(groundTruth, groundValue);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
