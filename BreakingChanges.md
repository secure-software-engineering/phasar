# Breaking Changes

## development HEAD

- The `DTAResolver` and the cli option `--call-graph-analysis=dta` do not work anymore (due to opaque pointers) and will be removed for the next release. Please use the `OTF` or `RTA` resolver instead.
- The default type-hierarchy implementation has been changed from `LLVMTypeHierarchy` to `DIBasedTypeHierarchy`. This also requires all affected analyses to be performed on LLVM IR that contains debug information.
- Removed the phasar-library `phasar_controller`. It is now part of the tool `phasar-cli`.
- The API of the `TypeHierarchy` interface (and thus the `LLVMTypeHierarchy` and `DIBasedTypeHierarchy` as well) has changed:
  - No handling of the super-type relation (only sub-types)
  - No VTable handling anymore -- has been out-sourced into `LLVMVFTableProvider`
  - minor API changes
- The constructors of the call-graph resolvers have changed. They:
  - take the `LLVMProjectIRDB` as pointer-to-const
  - take an additional second parameter of type `const LLVMVFTableProvider *`
  - not necessarily require a `LLVMTypeHierarchy` anymore
- Some constructors of `LLVMBasedICFG` do not accept a `LLVMTypeHierarchy` pointer anymore
- Removed IfdsFieldSensTaintAnalysis as it relies on LLVM's deprecated typed-pointers.

## v2403

- Versioning scheme has been changed from `<month><year>` to `<year><month>`
- Default build mode is no longer `SHARED` but `STATIC`. To build in shared mode, use the cmake option `BUILD_SHARED_LIBS` which we don't recommend anymore. Consider using `PHASAR_BUILD_DYNLIB` instead to build one big libphasar.so.
- Build type `DebugSan` has been removed in favor of a new CMake option `PHASAR_ENABLE_SANITIZERS` that not only works in `Debug` mode.

## v0323

- `EdgeFunctionPtrType` is no longer a `std::shared_ptr`. Instead `EdgeFunction<l_t>` should be used directly. `EdgeFunction` is now a *value-type* that encapsulates its memory management by itself.
- Concrete `EdgeFunction` types no longer derive from any base-class. Instead they just need to implement the required API functions. `EdgeFunction` implementations should me move-constructible and can be implicitly cast to `EdgeFunction`. To verify that your type implements the edge function interface use the `IsEdgeFunction` type trait. The API functions have been changed as follows:
  - All API functions of `EdgeFunction` must be `const` qualified.
  - `EdgeFunctionPtrType composeWith(EdgeFunctionPtrType SecondFunction)` and `EdgeFunctionPtrType joinWith(EdgeFunctionPtrType OtherFunction)` have been changed to `static EdgeFunction<l_t> compose(EdgeFunctionRef<T> This, const EdgeFunction<l_t>& SecondFunction)` and `static EdgeFunction<l_t> join(EdgeFunctionRef<T> This, const EdgeFunction<l_t>& OtherFunction)` respectively. Here, the `This` parameter models the former `shared_from_this()`.
  - `bool equal_to(EdgeFunctionPtrType Other)const` has been changed to `bool operator==(const T &Other)const noexcept`, where `T` is your concrete edge function type.
  - `void print(llvm::raw_ostream &OS, bool IsForDebug)` has been changed to `friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS, const T& EF)`.
- `EdgeFunction` is tagged with `[[clang::trivial_abi]]`. Hence, you should not rely on any destruction order within a top-level statement that uses temporary `EdgeFunction` objects.
- `EdgeFunctionSingletonFactory` has been removed. Use `EdgeFunctionSingletonCache` instead.
- `TaintConfig` has been renamed to `LLVMTaintConfig`. For generic code you may want to use the LLVM-independent `TaintConfigBase` CRTP interface instead.
- Renamed `phasar/PhasarLLVM/DataFlowSolver/` to either `phasar/DataFlow/` or `phasar/PhasarLLVM/DataFlow/` depending on whether the components need LLVMCore. Analoguous changes in `lib/` and `unittests/`.
    An incomplete list of moved/renamed files:
  - `phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/*` => `phasar/DataFlow/IfdsIde/Solver/*`
  - `phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IDETabulationProblem.h` => `phasar/DataFlow/IfdsIde/IDETabulationProblem.h`
  - `phasar/DB/LLVMProjectIRDB.h` => `phasar/PhasarLLVM/DB/LLVMProjectIRDB.h`
  - ...
- Renamed and split up some libraries:
  - `phasar_phasarllvm_utils` => `phasar_llvm_utils`
  - `phasar_typehierarchy` => `phasar_llvm_typehierarchy`
  - `phasar_ifdside` => `phasar_llvm_ifdside`
  - `phasar_controlflow` has its LLVM dependent stuff moved to `phasar_llvm_controlflow`
  - `phasar_db` has its LLVM dependent stuff moved to `phasar_llvm_db`
  - `phasar_pointer` has its LLVM dependent stuff moved to `phasar_llvm_pointer`
- Renamed the phasar tool `phasar-llvm` to `phasar-cli`
- `LLVMPointsTo[.*]` has been renamed to `LLVMAlias[.*]`
- The ctor of `LLVMAliasSet` now takes the `LLVMProjectIRDB` as pointer instead of a reference to better document that it may capture the IRDB by reference.
- The `PointsToInfo` interface has been replaced by the CRTP interface `AliasInfoBase`. Introduced two type-erased implementations of that interface: `AliasInfo` and `AliasInfoRef`. In most cases you should replace `PointsToInfo *` and `LLVMPointsToInfo *` by `AliasInfoRef`, bzw. `LLVMAliasInfoRef`.
- Introduced a new interface `PointsToInfoBase` and type-erased implementations `PointstoInfo` and `PointsToInfoRef`. Don't confuse them with the old `PointsToInfo`!!! (However, they have different APIs, so you should encounter compilation errors then)

## v1222

- Removed virtual interfaces `CFG<N,F>` and `ICFG<N,F>` and replaced by CRTP interfaces `CFGBase` and `ICFGBase`. Use the concrete types `LLVMBasedICFG` and `LLVMBasedCFG` instead. In template code you can use the type traits `is_crtp_base_of_v` and `is_icfg_v` to check for conformance to the interfaces.
- The `LLVMBasedICFG` now takes the IRDB as pointer instead of a reference to better document that it may capture the IRDB by reference.
- Renamed `ProjectIRDB` to `LLVMProjectIRDB` and added a generic CRTP interface `ProjectIRDBBase` that does not depend on LLVM
- Changed the API of `LLVMProjectIRDB`: The IRDB does no longer link multiple LLVM modules together, i.e. the ctor that reads a module from a file now takes a single filename instead of a vector. If you still want to link multiple LLVM modules together, use LLVM's Linker functionality instead. `ProjecIRDB::getAllModules()` has been removed and `ProjectIRDB::getWPAModule()` has been renamed to `LLVMProjectIRDB::getModule()`.
- The `LLVMProjectIRDB` ctor that takes a raw-pointer to `llvm::Module` does no longer preprocess the module (i.e. attaching metadata IDs to it). You can still explicitly use the `ValueAnnotationPass` to do so manually.
- The type `WholeProgramAnalysis` has been removed. Use `AnalysisController` instead.
- The IFDS and IDE TabulationProblems no longer take all of `LLVMProjectIRDB*`, `LLVMTypeHierarchy*`, `LLVMPointsToInfo*` and `LLVMBasedICFG*` as an argument. Instead, they only get what they need.
- The `IFDSSolver` and `IDESolver` now take an instance of the `ICFGBase` interface as additional argument to their ctor (because the analysis problems now not necessarily store a reference to it anymore).
- The `IDETabulationProblem` is now a base class of `IFDSTabulationProblem` (and not vice versa as it was previously). In their ctors they only take the bare minimum of arguments: The IRDB, the entrypoints and optionally the special zero-value. If the zero-value is not passed in the ctor (as it was previously), it has to be set from within the client analysis' ctor. You may use the new function `initializeZeroValue(d_t)` for this.
