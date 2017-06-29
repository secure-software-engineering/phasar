# =============================================================================
# Name        : Makefile
# Author      : Philipp D. Schubert
# Version     : 1.0
# Copyright   : see LICENSE.txt
# Description : Data Flow Anlysis for LLVM
# =============================================================================

# name for the executable file
EXE := main

# basic directories
BIN := bin/
SRC := src/
DOC := doc/
OUTPUT := output/
OBJ := obj/
DWO := dwo/
OUTPUT := output/

# important special files
MAIN_FILE := $(SRC)main.cpp

# source code directories
SRC_ANALYSIS := $(SRC)analysis/
SRC_CLANG := $(SRC)clang/
SRC_LIB := $(SRC)lib/
SRC_DB := $(SRC)db/
SRC_FLEX := $(SRC)flex/
SRC_UTILS := $(SRC)utils/
SRC_UNIT_TESTS := $(SRC)unit_tests/

# object code directories
OBJ_ANALYSIS := $(OBJ)analysis/
OBJ_CLANG := $(OBJ)clang/
OBJ_LIB := $(OBJ)lib/
OBJ_DB := $(OBJ)db/
OBJ_FLEX := $(OBJ)flex/
OBJ_UTILS := $(OBJ)utils/
OBJ_UNIT_TESTS := $(OBJ)unit_tests/

# important scripts and binaries
SCRIPT_AUTOFORMAT := misc/autoformat_sources.py

# compiler to use
CXX := clang++

# compiler flags
CXX_FLAGS := -std=c++14			# change to c++14 and libc++ Clang when possible
CXX_FLAGS += -stdlib=libstdc++ 	# libstdc++ for GCC, libc++ for Clang
CXX_FLAGS += -O0 #-O4
CXX_FLAGS += -Wno-unknown-warning-option # ignore unknown warnings (as '-Wno-maybe-uninitialized' resulting from a bug in 'llvm-config')
CXX_FLAGS += -pipe
CXX_FLAGS += -g
CXX_FLAGS += -rdynamic
CXX_FLAGS += -march=native
CXX_FLAGS += -Wall
CXX_FLAGS += -Wextra
CXX_FLAGS += -DNDEBUG

# sqlite3 library to link with
SQLITE3_LIBS := -lsqlite3

# boost libraries to link with
BOOST_LIBS := -lboost_filesystem -lboost_system -lboost_program_options

# llvm flags to use
# the LLVM_FLAGS usually contain '-fno-exceptions' we override that by '-fcxx-exceptions' to turn exceptions back on
LLVM_FLAGS :=  `llvm-config --cxxflags --ldflags` -fcxx-exceptions		# core support system analysis ipa jit mcjit native cppbackend`

# llvm libraries to link with
LLVM_LIBS := `llvm-config --system-libs --libs all`		# core support system analysis ipa jit mcjit native cppbackend`

# clang libraries to link with
CLANG_FLAGS := 	-lclangTooling\
		-lclangFrontendTool\
		-lclangFrontend\
		-lclangDriver\
		-lclangSerialization\
		-lclangCodeGen\
		-lclangParse\
		-lclangSema\
		-lclangStaticAnalyzerFrontend\
		-lclangStaticAnalyzerCheckers\
		-lclangStaticAnalyzerCore\
		-lclangAnalysis\
		-lclangARCMigrate\
		-lclangRewrite\
		-lclangRewriteFrontend\
		-lclangEdit\
		-lclangAST\
		-lclangASTMatchers\
		-lclangLex\
		-lclangBasic\
		`llvm-config --libs`\
		`llvm-config --system-libs`\
		-lcurses\

# definition of targets
.PHONY: clean

all: $(BIN)$(EXE)

# this target currently exists just for testing purposes
plugins:
	@echo "comiling plugins into shared object libraries ..."
	$(CXX) $(CXX_FLAGS) $(LLVM_FLAGS) -fPIC -shared -Wl,--no-undefined src/analysis/plugins/IFDSTabulationProblemTestPlugin.cpp -L$(LIB_CXX) $(LLVM_LIBS) $(BOOST_LIBS) -o src/analysis/plugins/IFDSTabulationProblemTestPlugin.so

doc:
	@echo "building the documentation of the source code ..."
	cd $(SRC); \
	doxygen doxy_config.conf
	@echo "built documentation"

format-code:
	@echo "formatting the project using clang-format ..."
	python3 $(SCRIPT_AUTOFORMAT)

clean:
	rm -rf $(BIN)
	rm -rf $(DOC)
	rm -rf $(OBJ)
	rm -rf $(DWO)

utils_header_list := $(shell find $(SRC_UTILS) -name "*.hh")
utils_impl_list := $(shell find $(SRC_UTILS) -name "*.cpp")
$(OBJ_UTILS): $(utils_header_list) $(utils_impl_list)
	mkdir -p $(OBJ_UTILS); \
	$(CXX) $(CXX_FLAGS) $(LLVM_FLAGS) -c $(utils_impl_list); \
	mv *.o $(OBJ_UTILS); \

db_header_list := $(shell find $(SRC_DB) -name "*.hh")
db_impl_list := $(shell find $(SRC_DB) -name "*.cpp")
$(OBJ_DB): $(db_header_list) $(db_impl_list)
	mkdir -p $(OBJ_DB); \
	$(CXX) $(CXX_FLAGS) $(LLVM_FLAGS) -c $(db_impl_list); \
	mv *.o $(OBJ_DB); \

clang_header_list := $(shell find $(SRC_CLANG) -name "*.hh")
clang_impl_list := $(shell find $(SRC_CLANG) -name "*.cpp")
$(OBJ_CLANG): $(clang_header_list) $(clang_impl_list)
	mkdir -p $(OBJ_CLANG); \
	$(CXX) $(CXX_FLAGS) $(LLVM_FLAGS) -c $(clang_impl_list); \
	mv *.o $(OBJ_CLANG); \

lib_header_list := $(shell find $(SRC_LIB) -name "*.hh")
lib_impl_list := $(shell find $(SRC_LIB) -name "*.cpp")
$(OBJ_LIB): $(lib_header_list) $(lib_impl_list)
	mkdir -p $(OBJ_LIB); \
	$(CXX) $(CXX_FLAGS) $(LLVM_FLAGS) -c $(lib_impl_list); \
	mv *.o $(OBJ_LIB); \

flex_header_list := $(shell find $(SRC_FLEX) -name "*.hh")
flex_impl_list := $(shell find $(SRC_FLEX) -name "*.cpp")
$(OBJ_FLEX): $(flex_header_list) $(flex_impl_list)
	mkdir -p $(OBJ_FLEX); \
	$(CXX) $(CXX_FLAGS) $(LLVM_FLAGS) -c $(flex_impl_list); \
	mv *.o $(OBJ_FLEX); \

analysis_header_list := $(shell find $(SRC_ANALYSIS) -name "*.hh")
analysis_impl_list := $(shell find $(SRC_ANALYSIS) -name "*.cpp")
$(OBJ_ANALYSIS): $(analysis_header_list) $(analysis_impl_list) 
	mkdir -p $(OBJ_ANALYSIS); \
	$(CXX) $(CXX_FLAGS) $(LLVM_FLAGS) -c $(analysis_impl_list); \
	mv *.o $(OBJ_ANALYSIS); \

# compile main.cpp and link all other object files with it to produce final executable
$(BIN)$(EXE): $(MAIN_FILE) $(OBJ_ANALYSIS) $(OBJ_CLANG) $(OBJ_DB) $(OBJ_FLEX) $(OBJ_LIB) $(OBJ_UTILS) 
	$(CXX) $(CXX_FLAGS) $(LLVM_FLAGS) \
	-L$(LIB_CXX) \
	$(OBJ_CLANG)*.o $(OBJ_DB)*.o $(OBJ_FLEX)*.o $(OBJ_LIB)*.o $(OBJ_UTILS)*.o $(OBJ_ANALYSIS)*.o \
	$(MAIN_FILE) \
	$(CLANG_FLAGS) $(LLVM_LIBS) $(BOOST_LIBS) $(SQLITE3_LIBS) -o $(EXE); \
	mkdir -p $(BIN); \
	mkdir -p $(DWO); \
	mv $(EXE) $(BIN); \
	mv *.dwo $(DWO); true;
	@echo "\ndone! ;-)"
