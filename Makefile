#/*****************************************************************************
#  * Copyright (c) 2017 Philipp Schubert.
#  * All rights reserved. This program and the accompanying materials are made 
#  * available under the terms of the TODO: add suitable License ... which 
#  * accompanies this distribution, and is available at
#  * http://www.addlicense.de
#  * 
#  * Contributors:
#  *     Philipp Schubert
#  ***************************************************************************/

# Set-up the compiler
CXX = clang++

# Set-up the basic compiler flags
CXX_FLAGS = -std=c++14
CXX_FLAGS += -Wall
CXX_FLAGS += -Wextra
CXX_FLAGS += -MMD
CXX_FLAGS += -MP
CXX_FLAGS += -stdlib=libstdc++ 	# libstdc++ for GCC, libc++ for Clang
CXX_FLAGS += -O3 #-O4
CXX_FLAGS += -march=native
CXX_FLAGS += -fPIC
CXX_FLAGS += -Wno-unknown-warning-option # ignore unknown warnings (as '-Wno-maybe-uninitialized' resulting from a bug in 'llvm-config')
CXX_FLAGS += -Qunused-arguments # ignore unused compiler arguments
CXX_FLAGS += -pipe
#CXX_FLAGS += -g
CXX_FLAGS += -rdynamic
CXX_FLAGS += -DNDEBUG
CXX_FLAGS += -DBOOST_LOG_DYN_LINK

# Add header search paths
CXX_INCL = -I ./json/src/

# Define the google test run parameters
GTEST_RUN_PARAMS = --gtest_repeat=1
GTEST_RUN_PARAMS += --gtest_filter=StoreLLVMTypeHierarchyTest.*

# Define useful make functions
recwildcard=$(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call recwildcard,$d/,$2))

# Name of the executable
EXE = main

# Basic project directories
BIN = bin/
OBJDIR = obj/
LLVMDIR = llvmir/
DOC = doc/
SRC = src/
TEST = tests/
PLUGINDIR = src/analysis/plugins/
PLUGINSODIR = so/
ALL_SRC = $(sort $(dir $(call recwildcard,$(SRC)**/*/)))

# Set the virtual (search) path
VPATH = $(SRC):$(ALL_SRC)

# Determine all object files that we need in order to produce an executable
OBJ = $(addprefix $(OBJDIR),$(notdir $(patsubst %.cpp,%.o,$(call recwildcard,src/,*.cpp))))

# Determine all llvm files that we need in order to produce the complete llvm ir code
LLVMIR = $(addprefix $(LLVMDIR),$(notdir $(patsubst %.cpp,%.ll,$(call recwildcard,src/,*.cpp))))

# Determine all test files, that we are given
TST = $(call recwildcard,$(TEST),*.cpp)

# Determine the names of the test executables
TSTEXE = $(patsubst %.cpp,%,$(TST))

# Determine all shared object files that we need to produce in order to process the plugins
SO = $(addprefix $(PLUGINSODIR),$(notdir $(patsubst %.cxx,%.so,$(call recwildcard,src/,*.cxx))))

# To determine source (.cpp) and header (.h) dependencies
DEP = $(OBJ:.o=.d)

# Important scripts and binaries to use
SCRIPT_AUTOFORMAT := misc/autoformat_sources.py

# Further llvm compiler flags
LLVM_FLAGS :=  `llvm-config --cxxflags --ldflags` -fcxx-exceptions -std=c++14
# Thread model to use
THREAD_MODEL := -pthread
# Libraries to link against
SOL_LIBS := -ldl
SQLITE3_LIBS := -lsqlite3
MYSQL_LIBS := -lmysqlcppconn
CURL_LIBS := -lcurl
GTEST_LIBS := -lgtest
BOOST_LIBS := -lboost_filesystem
BOOST_LIBS += -lboost_system
BOOST_LIBS += -lboost_program_options
BOOST_LIBS += -lboost_log
BOOST_LIBS += -lboost_thread
BOOST_LIBS += -lboost_graph
LLVM_LIBS := `llvm-config --system-libs --libs all`
CLANG_LIBS := -lclangTooling
CLANG_LIBS +=	-lclangFrontendTool
CLANG_LIBS += -lclangFrontend
CLANG_LIBS +=	-lclangDriver
CLANG_LIBS +=	-lclangSerialization
CLANG_LIBS +=	-lclangCodeGen
CLANG_LIBS +=	-lclangParse
CLANG_LIBS +=	-lclangSema
CLANG_LIBS +=	-lclangStaticAnalyzerFrontend
CLANG_LIBS +=	-lclangStaticAnalyzerCheckers
CLANG_LIBS +=	-lclangStaticAnalyzerCore
CLANG_LIBS +=	-lclangAnalysis
CLANG_LIBS +=	-lclangARCMigrate
CLANG_LIBS +=	-lclangRewrite
CLANG_LIBS +=	-lclangRewriteFrontend
CLANG_LIBS +=	-lclangEdit
CLANG_LIBS +=	-lclangAST
CLANG_LIBS +=	-lclangASTMatchers
CLANG_LIBS +=	-lclangLex
CLANG_LIBS +=	-lclangBasic
CLANG_LIBS +=	-lcurses

.PHONY: clean

all: $(OBJDIR) $(BIN) $(BIN)$(EXE)

# To resolve header dependencies
-include $(DEP)

$(OBJDIR):
	mkdir $@

$(LLVMDIR):
	mkdir $@

$(BIN):
	mkdir $@

$(BIN)$(EXE): $(OBJ)
	@echo "linking into executable file ..."
	$(CXX) $(CXX_FLAGS) $(CXX_INCL) $^ $(SOL_LIBS) $(CLANG_LIBS) $(LLVM_LIBS) $(BOOST_LIBS) $(SQLITE3_LIBS) $(MYSQL_LIBS) $(CURL_LIBS) -o $@ $(THREAD_MODEL)
	@echo "done ;-)"

$(OBJDIR)%.o: %.cpp
	$(CXX) $(CXX_FLAGS) $(CXX_INCL) $(LLVM_FLAGS) -c $< -o $@

doc:
	@echo "building the documentation of the source code ..."
	cd $(SRC); \
	doxygen doxy_config.conf
	@echo "built documentation"

clang-format-code:
	@echo "formatting the project using clang-format ..."
	python3 $(SCRIPT_AUTOFORMAT)

$(PLUGINSODIR):
	mkdir $@

plugins: $(PLUGINSODIR) $(SO)

$(PLUGINSODIR)%.so: %.cxx
	$(CXX) $(CXX_FLAGS) $(CXX_INCL) $(LLVM_FLAGS) -shared obj/ZeroValue.o $< -o $@ 

tests: $(TSTEXE) $(TST)

$(TSTEXE): %: %.cpp $(filter-out obj/main.o,$(OBJ))
	@echo "Compile test: $@"
	$(CXX) $(CXX_FLAGS) $(CXX_INCL) $(LLVM_FLAGS) $^ $(CLANG_LIBS) $(LLVM_LIBS) $(BOOST_LIBS) $(SQLITE3_LIBS) $(MYSQL_LIBS) $(CURL_LIBS) -o $@ $(GTEST_LIBS) $(THREAD_MODEL)

run_tests: tests
	@echo "Run tests: $(TSTEXE)"
	$(foreach test,$(TSTEXE),./$(test) $(GTEST_RUN_PARAMS);)

llvmir: $(LLVMDIR) $(LLVMIR)

$(LLVMDIR)%.ll: %.cpp
	$(CXX) $(CXX_FLAGS) $(CXX_INCL) $(LLVM_FLAGS) -emit-llvm -S $< -o $@

hello:
	@echo "Hello World!"

clean_plugins:
	rm -rf $(PLUGINSODIR)

clean_tests:
	rm -f $(TSTEXE)
	rm -f $(patsubst %,%.d,$(TSTEXE))

clean_llvm:
	rm -rf $(LLVMDIR)

clean: clean_plugins clean_tests clean_llvm
	rm -rf $(BIN)
	rm -rf $(OBJDIR)
	rm -rf $(DOC)
