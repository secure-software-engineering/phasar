#/*****************************************************************************
#  * Copyright (c) 2017 Philipp Schubert.
#  * All rights reserved. This program and the accompanying materials are made 
#  * available under the terms of LICENSE.txt.
#  * 
#  * Contributors:
#  *     Philipp Schubert
#  ***************************************************************************/

# Set-up the compiler and OS variables
GCC = g++
CLANG = clang++
# Clang is used as the default compiler, but the compiler can be changed using
# the commandline when calling make, e.g. '$ make -j4 CXX=g++' can be specified
# in order to use the GCC compiler rather than Clang.
CXX = $(CLANG) 
SUPPORTED_COMPILERS = $(CLANG) $(GCC)
OS = $(shell uname -s)
LINUX = Linux
MAC = Darwin

# Set-up the basic compiler flags
CXX_FLAGS = -std=c++14
CXX_FLAGS += -Wall
CXX_FLAGS += -Wextra
CXX_FLAGS += -Wpedantic
CXX_FLAGS += -MMD
CXX_FLAGS += -MP
# Use libstdc++ for GCC's - and libc++ for Clang's STL implementation
ifneq ($(CXX),$(GCC))
ifeq ($(OS),$(LINUX))
CXX_FLAGS += -stdlib=libstdc++
else ifeq ($(OS),$(MAC))
CXX_FLAGS += -stdlib=libc++
endif
endif
CXX_FLAGS += -fdata-sections
CXX_FLAGS += -ffunction-sections
CXX_FLAGS += -O0 # -O4
# CXX_FLAGS += -march=native
# CXX_FLAGS += -DNDEBUG
# CXX_FLAGS += -flto=full # or use: -flto=thin
# CXX_FLAGS += -fuse-ld=gold 
CXX_FLAGS += -fPIC
CXX_FLAGS += -Wno-unused-variable
CXX_FLAGS += -Wno-unused-parameter
ifneq ($(CXX),$(GCC))
CXX_FLAGS += -Wno-unknown-warning-option
CXX_FLAGS += -Qunused-arguments
endif
CXX_FLAGS += -pipe
CXX_FLAGS += -g
CXX_FLAGS += -rdynamic
CXX_FLAGS += -DBOOST_LOG_DYN_LINK
ifeq ($(OS),$(LINUX))
CXX_FLAGS += -L/usr/local/
CXX_FLAGS += -L/usr/local/
else ifeq ($(OS),$(MAC))
CXX_FLAGS += -L/usr/local/opt/boost/lib
CXX_FLAGS += -L/usr/local/opt/llvm@3.9/lib
endif

# Add header search paths
CXX_INCL = -I ./lib/json/src/

# Define the google test run parameters
GTEST_RUN_PARAMS = --gtest_repeat=3

# Define useful make functions
recwildcard=$(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call recwildcard,$d/,$2))

# Name of the executable
EXE = main
# Suffix for the test executables
TEST_SUFFIX = .test

# Basic project directories
BIN = bin/
OBJDIR = obj/
LLVMDIR = llvmir/
DOC = doc/
SRC = src/
LIBS = lib/
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
#LLVM_FLAGS :=  `llvm-config-3.9 --cxxflags --ldflags` -fcxx-exceptions -std=c++14 -O0 -g
# `llvm-config-3.9 --cxxflags --ldflags` usually gives:
# -I/usr/lib/llvm-3.9/include -std=c++0x -gsplit-dwarf -Wl,-fuse-ld=gold -fPIC -fvisibility-inlines-hidden -Wall -W -Wno-unused-parameter -Wwrite-strings -Wcast-qual -Wno-missing-field-initializers -pedantic -Wno-long-long -Wno-maybe-uninitialized -Wdelete-non-virtual-dtor -Wno-comment -Werror=date-time -std=c++11 -ffunction-sections -fdata-sections -O2 -g -DNDEBUG -fno-exceptions -D_GNU_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -L/usr/lib/llvm-3.9/lib

LLVM_FLAGS = -fvisibility-inlines-hidden
ifeq ($(OS),$(LINUX))
LLVM_FLAGS += -I/usr/lib/llvm-3.9/include
LLVM_FLAGS += -L/usr/lib/llvm-3.9/lib 
else ifeq ($(OS),$(MAC))
LLVM_FLAGS += -I/usr/lib/llvm-3.9/include
LLVM_FLAGS += -L/usr/lib/llvm-3.9/lib 
endif
LLVM_FLAGS += -D_GNU_SOURCE 
LLVM_FLAGS += -D__STDC_CONSTANT_MACROS 
LLVM_FLAGS += -D__STDC_FORMAT_MACROS 
LLVM_FLAGS += -D__STDC_LIMIT_MACROS

# Thread model to use
THREAD_MODEL := -pthread
# Libraries to link against
SOL_LIBS := -ldl
SQLITE3_LIBS := -lsqlite3
MYSQL_LIBS := -lmysqlcppconn
CURL_LIBS := -lcurl
GTEST_LIBS := -lgtest
ifeq ($(OS),$(LINUX))
BOOST_LIBS := -lboost_filesystem
BOOST_LIBS += -lboost_system
BOOST_LIBS += -lboost_program_options
BOOST_LIBS += -lboost_log
BOOST_LIBS += -lboost_thread
else ifeq ($(OS),$(MAC))
BOOST_LIBS := -lboost_filesystem-mt
BOOST_LIBS += -lboost_system-mt
BOOST_LIBS += -lboost_program_options-mt
BOOST_LIBS += -lboost_log-mt
BOOST_LIBS += -lboost_thread-mt
endif
LLVM_LIBS := `llvm-config-3.9 --system-libs --libs all`
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

all: check-used-compiler $(OBJDIR) $(BIN) $(BIN)$(EXE)

check-used-compiler:
ifneq (,$(findstring $(CXX),$(SUPPORTED_COMPILERS)),)
	$(info $(CXX) compiler is supported.)
else
	$(error Compiler is not supported! Use $(CLANG) or $(GCC) instead.)
endif

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
	$(CXX) $(CXX_FLAGS) $(CXX_INCL) $(LLVM_FLAGS) $^ $(SOL_LIBS) $(CLANG_LIBS) $(LLVM_LIBS) $(BOOST_LIBS) $(SQLITE3_LIBS) $(MYSQL_LIBS) $(CURL_LIBS) -o $@ $(THREAD_MODEL)
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
	$(CXX) $(CXX_FLAGS) $(CXX_INCL) $(LLVM_FLAGS) -fPIC -shared obj/ZeroValue.o $< -o $@ 

tests: $(OBJDIR) $(OBJ) gtest $(TSTEXE) $(TST) $(OBJ)

$(TSTEXE): %: %.cpp $(filter-out obj/main.o,$(OBJ))
	@echo "Compile test: $@"
	$(CXX) $(CXX_FLAGS) $(CXX_INCL) $(CPPFLAGS) $(GTEST_FLAGS) $(LLVM_FLAGS) $^ $(CLANG_LIBS) $(LLVM_LIBS) $(BOOST_LIBS) $(SQLITE3_LIBS) $(MYSQL_LIBS) $(CURL_LIBS) -o $@$(TEST_SUFFIX) $(GTEST_LIBS) $(THREAD_MODEL)

run-tests: tests
	@echo "Run tests: $(TSTEXE)"
	$(foreach test,$(TSTEXE),./$(test)$(TEST_SUFFIX) $(GTEST_RUN_PARAMS);)

llvmir: $(LLVMDIR) $(LLVMIR)

$(LLVMDIR)%.ll: %.cpp
	$(CXX) $(CXX_FLAGS) $(CXX_INCL) $(LLVM_FLAGS) -emit-llvm -S $< -o $@

hello:
	@echo "Hello, World!"

# Targets to build gtest (this is a modified version of 'googletest/googletest/make/Makefile')

GTEST_DIR = $(LIBS)googletest/googletest/

GTEST_FLAGS := -L$(LIBS)/gtest/

# Flags passed to the preprocessor.
# Set Google Test's header directory as a system directory, such that
# the compiler doesn't generate warnings in Google Test headers.
CPPFLAGS += -isystem $(GTEST_DIR)/include

# All Google Test headers.  Usually you shouldn't change this
# definition.
GTEST_HEADERS = $(GTEST_DIR)/include/gtest/*.h \
                $(GTEST_DIR)/include/gtest/internal/*.h

# House-keeping build targets.

# Usually you shouldn't tweak such internal variables, indicated by a
# trailing _.
GTEST_SRCS_ = $(GTEST_DIR)src/*.cc $(GTEST_DIR)src/*.h $(GTEST_HEADERS)

# Building the gtest library
gtest: gtest.a
	mkdir -p $(LIBS)$@
	mv gtest.a libgtest.a
	mv gtest-* libgtest.a $(LIBS)$@

# For simplicity and to avoid depending on Google Test's
# implementation details, the dependencies specified below are
# conservative and not optimized.  This is fine as Google Test
# compiles fast and for ordinary users its source rarely changes.
gtest-all.o : $(GTEST_SRCS_)
	$(CXX) $(CPPFLAGS) -I$(GTEST_DIR) $(CXX_FLAGS) -c \
            $(GTEST_DIR)src/gtest-all.cc

gtest_main.o : $(GTEST_SRCS_)
	$(CXX) $(CPPFLAGS) -I$(GTEST_DIR) $(CXX_FLAGS) -c \
            $(GTEST_DIR)src/gtest_main.cc

gtest.a : gtest-all.o
	$(AR) $(ARFLAGS) $@ $^

gtest_main.a : gtest-all.o gtest_main.o
	$(AR) $(ARFLAGS) $@ $^

# Targets to perform the clean-up

clean-gtest :
	rm -rf $(LIBS)gtest/

clean-plugins:
	rm -rf $(PLUGINSODIR)

clean-tests: clean-gtest
	rm -f $(patsubst %,%$(TEST_SUFFIX),$(TSTEXE))
	rm -f $(patsubst %,%.d,$(TSTEXE))

clean-llvm:
	rm -rf $(LLVMDIR)

clean: clean-plugins clean-tests clean-llvm
	rm -rf $(BIN)
	rm -rf $(OBJDIR)
	rm -rf $(DOC)
