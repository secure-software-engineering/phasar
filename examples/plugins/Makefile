CXX := clang++
CXX_FLAGS := -std=c++14
CXX_FLAGS += -Wall
CXX_FLAGS += -Wno-return-type-c-linkage
CXX_FLAGS += -O0
CXX_FLAGS += -fPIC
CXX_FLAGS += -shared

ICFGPluginSrc := MyICFGPlugin.cxx
IFDSPluginSrc := MyIFDSProblem.cxx
ICFGPlugin := $(patsubst %.cxx,%.so,$(ICFGPluginSrc))
IFDSPlugin := $(patsubst %.cxx,%.so,$(IFDSPluginSrc))

all: ifdsproblem callgraph

ifdsproblem:
	$(CXX) $(CXX_FLAGS) $(IFDSPluginSrc) -o $(IFDSPlugin)

callgraph:
	$(CXX) $(CXX_FLAGS) $(ICFGPluginSrc) -o $(ICFGPlugin)

clean:
	rm -f $(ICFGPlugin)
	rm -f $(IFDSPlugin)
	rm -f *.dot
	rm -f *.json
