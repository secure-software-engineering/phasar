###############################################################################
# Name        : Makefile                                                      #
# Author      : Philipp D. Schubert                                           #
# Version     : 2.0                                                           #
# Copyright   : see LICENSE.txt                                               #
# Description : Data Flow Anlysis for LLVM                                    #
###############################################################################

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
CXX_FLAGS += -Wno-unknown-warning-option # ignore unknown warnings (as '-Wno-maybe-uninitialized' resulting from a bug in 'llvm-config')
CXX_FLAGS += -Qunused-arguments # ignore unused compiler arguments
CXX_FLAGS += -pipe
CXX_FLAGS += -g
CXX_FLAGS += -rdynamic
CXX_FLAGS += -DNDEBUG

# Add header search paths
CXX_INCL = -I ./json/src/

# Define useful make functions
recwildcard=$(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call recwildcard,$d/,$2))

# Name of the executable
EXE = main

# Basic project directories
BIN = bin/
OBJDIR = obj/
DOC = doc/
SRC = src/
ALL_SRC = $(sort $(dir $(call recwildcard,$(SRC)**/*/)))

# Set the virtual (search) path
VPATH = $(SRC):$(ALL_SRC)

# Determine all object files that we need in order to produce an executable
OBJ = $(addprefix $(OBJDIR),$(notdir $(patsubst %.cpp,%.o,$(call recwildcard,src/,*.cpp))))

# To determine source (.cpp) and header (.hh) dependencies
DEP = $(OBJ:.o=.d)

# Important scripts and binaries to use
SCRIPT_AUTOFORMAT := misc/autoformat_sources.py

# Further llvm compiler flags
LLVM_FLAGS :=  `llvm-config --cxxflags --ldflags` -fcxx-exceptions

# Libraries to link against
SQLITE3_LIBS := -lsqlite3
BOOST_LIBS := -lboost_filesystem -lboost_system -lboost_program_options
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

$(BIN):
	mkdir $@

$(BIN)$(EXE): $(OBJ)
	$(CXX) $(CXX_FLAGS) $^ $(CLANG_LIBS) $(LLVM_LIBS) $(BOOST_LIBS) $(SQLITE3_LIBS) -o $@
	@echo "done ;-)"

$(OBJDIR)%.o: %.cpp
	$(CXX) $(CXX_FLAGS) $(CXX_INCL) $(LLVM_FLAGS) -c $< -o $@

doc:
	@echo "building the documentation of the source code ..."
	cd $(SRC); \
	doxygen doxy_config.conf
	@echo "built documentation"

format-code:
	@echo "formatting the project using clang-format ..."
	python3 $(SCRIPT_AUTOFORMAT)

# this target currently exists just for testing purposes
# plugins:
#	@echo "comiling plugins into shared object libraries ..."
#	$(CXX) $(CXX_FLAGS) $(LLVM_FLAGS) -fPIC -shared -Wl,--no-undefined src/analysis/plugins/IFDSTabulationProblemTestPlugin.cpp -L$(LIB_CXX) $(LLVM_LIBS) $(BOOST_LIBS) -o src/analysis/plugins/IFDSTabulationProblemTestPlugin.so

hello:
	@echo "Hello World!"

clean-db-dot:
	rm *.dot
	rm ptg_hexastore.db
	rm llheros_analyzer.db

clean:
	rm -rf $(BIN)
	rm -rf $(OBJDIR)
	rm -rf $(DOC)
	rm *.dot
