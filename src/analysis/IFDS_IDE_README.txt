IDE / IFDS Solver HEROS ported to C++
=====================================

In order to define your own interprocedural analysis in llheros you have to provide
your own implementation for the following classes. Some of the classes have to 
inherit from the generic base classes of llheros and override all of their pure virtual methods.


The type of nodes in the interprocedural control-flow graph.
class N

The type of data-flow facts to be computed by the tabulation problem.
class D

The type of objects used to represent methods.
class M

The type of values to be computed along flow edges.
class V

The type of inter-procedural control-flow graph being used.
class I - extends ICFG
In our case I is LLVMBasedInterproceduralICFG when we are working on LLVM IR.