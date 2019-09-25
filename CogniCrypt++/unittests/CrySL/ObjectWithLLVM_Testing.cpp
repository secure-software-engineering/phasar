#include<../../utils/logger.h>
#include<CrySLParser.h>
#include<formatConverter/ObjectWithLLVM.h>
#include<Parser/CrySLParserEngine.h>
#include<filesystem>
#include<gtest/gtest.h>
#include<iostream>

using namespace CCPP;

/*=========================TEST FIXTURE ======================*/

class ObjectWithLLVMTest : public ::testing::Test {

protected:
const std::string pathToCrySLFiles = "../../../test/CrySL";

ObjectWithLLVM *ObjectWithLLVM = 0;

ObjectWithLLVM() {}
virtual ~ObjectWithLLVM() {}

void Initialize(const std::string &CrySLFiles) {

for(const auto &file : CrySLFiles) {

EXPECT_EQ(std::filesystem::exists(file), true)<<file;
}

CrySLParserEngine = new cryslparserengine(CrySLFiles);
cryslparserengine->parseAndTypecheck();
}

void SetUp() override {bl::core::get()->set_logging_enabled(false);}

void TearDown() override{
	if (cryslparserengine){
		delete cryslparserengine;
		cryslparserengine = nullptr;
	}
}

void compareResults(bool groundTruth, size_t groundvalue) 
//TODO unittests
