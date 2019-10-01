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

  CrySLParserEngine *cryslparserengine = 0;

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

TEST_F(TypeCheckTest, TypeCheck_EVP_Cipher_CTX) {
  Initialize({pathToCrySLFiles + "EVP_CIPHER_CTX.cryptosl"});
  ASSERT_EQ(cryslparserengine == nullptr, false) << "CrySLParserEngine is null";
  bool groundTruth = true;
  bool typeCheckResult = cryslparserengine->typecheckSucceeded();
  size_t expectedErrors = 0;
  size_t actualErrors = cryslparserengine->getNumberOfSyntaxErrors();
  EXPECT_EQ(actualErrors, expectedErrors);
  EXPECT_EQ(typeCheckResult, groundTruth);
}

TEST_F(TypeCheckTest, TypeCheck_EVP_Cipher) {
  Initialize({pathToCrySLFiles + "EVP_CIPHER.cryptosl"});
  ASSERT_EQ(cryslparserengine == nullptr, false) << "CrySLParserEngine is null";
  bool groundTruth = true;
  bool constraintErrors = cryslparserengine->constraintsTypecheckSucceeded();
  EXPECT_EQ(constraintErrors, groundTruth);
  bool typeCheckResult = cryslparserengine->typecheckSucceeded();
  EXPECT_EQ(typeCheckResult, groundTruth);
}

TEST_F(TypeCheckTest, TypeCheck_EVP_MD_CTX) {
  Initialize({pathToCrySLFiles + "EVP_MD.cryptosl"});
  bool groundTruth = true;
  bool eventErrors = cryslparserengine->eventsTypecheckSucceeded();
  bool ensuresErrors = cryslparserengine->constraintsTypecheckSucceeded();
  EXPECT_EQ(eventErrors, groundTruth);
  EXPECT_EQ(ensuresErrors, groundTruth);
  bool typeCheckResult = cryslparserengine->typecheckSucceeded();
  EXPECT_EQ(typeCheckResult, groundTruth);
}

TEST_F(TypeCheckTest, TypeCheck_EVP_MD_incorrect) {
  Initialize({pathToCrySLFiles + "EVP_MD_incorrect.cryptosl"});
  bool groundTruth = true;
  bool orderErrors = cryslparserengine->orderTypecheckSucceeded();
  EXPECT_EQ(orderErrors, groundTruth);
  bool typeCheckResult = cryslparserengine->typecheckSucceeded();
  EXPECT_EQ(typeCheckResult, groundTruth);
}

TEST_F(TypeCheckTest, TypeCheck_EVP_KDF_CTX) {
  Initialize({pathToCrySLFiles + "EVP_KDF_CTX.cryptosl"});
  bool groundTruth = true;
  bool objectErrors = cryslparserengine->objectsTypecheckSucceeded();
  EXPECT_EQ(objectErrors, groundTruth);
  bool typeCheckResult = cryslparserengine->typecheckSucceeded();
  EXPECT_EQ(typeCheckResult, groundTruth);
}

TEST_F(TypeCheckTest, TypeCheck_EVP_Seal) {
  Initialize({pathToCrySLFiles + "EVP_Seal.cryptosl"});
  bool groundTruth = true;
  bool objectErrors = cryslparserengine->objectsTypecheckSucceeded();
  EXPECT_EQ(objectErrors, groundTruth);
  bool orderErrors = cryslparserengine->orderTypecheckSucceeded();
  EXPECT_EQ(orderErrors, groundTruth);
  bool typeCheckResult = cryslparserengine->typecheckSucceeded();
  EXPECT_EQ(typeCheckResult, groundTruth);
}

TEST_F(TypeCheckTest, TypeCheck_EVP_Calculate_HMAC) {
  Initialize({pathToCrySLFiles + "EVP_Calculate_HMAC.cryptosl"});
  bool groundTruth = true;
  bool eventsErrors = cryslparserengine->eventsTypecheckSucceeded();
  EXPECT_EQ(eventsErrors, groundTruth);
  bool ensuresErrors = cryslparserengine->ensuresTypecheckSucceeded();
  EXPECT_EQ(ensuresErrors, groundTruth);
  bool typeCheckResult = cryslparserengine->typecheckSucceeded();
  EXPECT_EQ(typeCheckResult, groundTruth);
}

TEST_F(TypeCheckTest, TypeCheck_EVP_Calculate_HMAC) {
  Initialize({pathToCrySLFiles + "EVP_Calculate_HMAC.cryptosl"});
  bool groundTruth = true;
  bool eventsErrors = cryslparserengine->eventsTypecheckSucceeded();
  EXPECT_EQ(eventsErrors, groundTruth);
  bool ensuresErrors = cryslparserengine->ensuresTypecheckSucceeded();
  EXPECT_EQ(ensuresErrors, groundTruth);
  bool typeCheckResult = cryslparserengine->typecheckSucceeded();
  EXPECT_EQ(typeCheckResult, groundTruth);
}

TEST_F(TypeCheckTest, TypeCheck_LBS_KeyPairGeneration) {
  Initialize({pathToCrySLFiles + "KeyPairGeneration.cryptosl"});
  bool groundTruth = true;
  bool requiresErrors = cryslparserengine->requiresTypecheckSucceeded();
  EXPECT_EQ(requiresErrors, groundTruth);
  bool ensuresErrors = cryslparserengine->ensuresTypecheckSucceeded();
  EXPECT_EQ(ensuresErrors, groundTruth);
  bool typeCheckResult = cryslparserengine->typecheckSucceeded();
  EXPECT_EQ(typeCheckResult, groundTruth);
}

TEST_F(TypeCheckTest, TypeCheck_LBS_PublicKey_Signature) {
  Initialize({pathToCrySLFiles + "PublicKey_Signature.cryptosl"});
  bool groundTruth = true;
  bool orderErrors = cryslparserengine->orderTypecheckSucceeded();
  EXPECT_EQ(orderErrors, groundTruth);
  bool requiresErrors = cryslparserengine->requiresTypecheckSucceeded();
  EXPECT_EQ(requiresErrors, groundTruth);
  bool typeCheckResult = cryslparserengine->typecheckSucceeded();
  EXPECT_EQ(typeCheckResult, groundTruth);
}

TEST_F(TypeCheckTest, TypeCheck_LBS_Stream_Decryption) {
  Initialize({pathToCrySLFiles + "Stream_Decryption.cryptosl"});
  bool groundTruth = true;
  bool requiresErrors = cryslparserengine->requiresTypecheckSucceeded();
  EXPECT_EQ(requiresErrors, groundTruth);
  bool typeCheckResult = cryslparserengine->typecheckSucceeded();
  EXPECT_EQ(typeCheckResult, groundTruth);
}

TEST_F(TypeCheckTest, TypeCheck_LBS_PublicKey_Encryption) {
  Initialize({pathToCrySLFiles + "PublicKeyEncryption.cryptosl"});
  bool groundTruth = true;
  bool requiresErrors = cryslparserengine->requiresTypecheckSucceeded();
  EXPECT_EQ(requiresErrors, groundTruth);
  bool typeCheckResult = cryslparserengine->typecheckSucceeded();
  EXPECT_EQ(typeCheckResult, groundTruth);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
