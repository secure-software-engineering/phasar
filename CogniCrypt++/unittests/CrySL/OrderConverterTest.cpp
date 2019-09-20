#include <../../utils/Logger.h>
#include <CrySLParser.h>
#include <FormatConverter/OrderConverter.h>
#include <Parser/CrySLParserEngine.h>
#include <filesystem>
#include <gtest/gtest.h>
#include <iostream>

using namespace CCPP;

/* ============== TEST FIXTURE ============== */

class OrderConverterTest : public ::testing::Test {

protected:
  const std::string pathToCrySLFiles = "../../../test/CrySL/";

  OrderConverter *orderConverter = 0;

  TypeCheckTest() {}
  virtual ~TypeCheckTest() {}

  void Initialize(const std::string &CrySLFile) {

    EXPECT_EQ(std::filesystem::exists(CrySLFile), true) << file;

    CrySLParserEngine cryslparserengine({CrySLFile});
    EXPECT_EQ(cryslparserengine->parseAndTypecheck(), true);
    auto ast = cryslparserengine.getAllASTs()[0];
    auto specname = ast->getAST()->spec()->qualifiedName()->getText();
    auto order = ast->getAST()->order();
    auto evts = ast->getAST()->events();

    orderConverter = new OrderConverter(specname, order, evts);
  }

  void SetUp() override { bl::core::get()->set_logging_enabled(false); }

  void TearDown() override {
    if (orderConverter) {
      delete orderConverter;
      orderConverter = nullptr;
    }
  }

  // TODO: compare results
  /*void compareResults(bool groundTruth, size_t groundValue) {
    ASSERT_EQ(cryslparserengine == nullptr, false)
        << "CrySLParserEngine is null";
    size_t numErrors = cryslparserengine->getNumberOfSyntaxErrors();
    bool typeCheckResult = cryslparserengine->typecheckSucceeded();

    EXPECT_EQ(numErrors, groundValue);
    EXPECT_EQ(typeCheckResult, groundTruth);
  }*/

}; // class Fixture

// TODO: actual unittests