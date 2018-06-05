CXX := clang++
CXX_FLAGS := -std=c++14
CXX_FLAGS += -Wall
CXX_FLAGS += -Wno-return-type-c-linkage
CXX_FLAGS += -O0
CXX_FLAGS += -fPIC
CXX_FLAGS += -shared
LLVM_FLAGS := `llvm-config --cppflags`

IFDSPluginSrc := MyIFDSProblem.cxx
IFDSPlugin := $(patsubst %.cxx,%.so,$(IFDSPluginSrc))

all: ifdsproblem

ifdsproblem:
	$(CXX) $(CXX_FLAGS) $(LLVM_FLAGS) $(IFDSPluginSrc) -o $(IFDSPlugin)

clean:
	rm -f $(IFDSPlugin)
	rm -f *.dot
	rm -f *.json
