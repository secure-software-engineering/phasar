#!/bin/bash

json="$1"
if [ -z "$1" ]; then
echo "preparing help..."
components="$(timeout 20 find ~/.conan/data/llvm/ -name components.json || echo "no components.json found in ~/conan/data/llvm/")"
echo "
invocation:
script path/to/components.json llvm-lib1 llvm-lib2 ...
 => resolves dependencies from llvm-lib1 and llvm-lib2, correct the link order and outputs it to \$(pwd)/resolve.log
script \"/home/.../llvm/14.0.6/.../package/.../lib/components.json\" LLVMPasses LLVMRemarks
 
Maybe you want to use one of the following components.json:
$components


Use this script if you have link errors with llvm where you are sure all targets are added.
Just check llvm-project for the first undefined reference in which target it is.
This script will get all transitive dependencies and also orders them.
Important: Iam assuming I have the correct order in this script but iam not 100% sure.

If you have link issues in consuming projects (phasar-llvm fine but phasar-llvm-test not), e.g. libLLVMCore.a undefined reference to xy(which is in LLVMRemarks), add LLVMCore followed by LLVMRemarks to this target.
I have no idea why this happens but it should fix it.
"
    exit 1
fi
shift
llvm=("$@")

set -euo pipefail

# assuming this order is fine, if not reorder
# last cmake build -> first cmake build
order=(LLVMTableGenGlobalISel LLVMTableGen LLVMFileCheck LLVMAMDGPUTargetMCA LLVMCFIVerify LLVMSymbolize LLVMDWARFLinker LLVMDWP LLVMDebugInfoGSYM LLVMDebuginfod LLVMDiff LLVMDlltoolDriver LLVMExegesisAArch64 LLVMExegesisMips LLVMExegesisPowerPC LLVMExegesisX86 LLVMExegesis LLVMObjectYAML LLVMFrontendOpenACC LLVMFuzzMutate LLVMInterfaceStub LLVMInterpreter LLVMX86TargetMCA LLVMMCA LLVMXRay clangAPINotes omp clangApplyReplacements clangChangeNamespace clangToolingASTDiff clangDoc clangHandleCXX clangAnalysisFlowSensitive clangDaemonTweaks clangDaemon clangToolingSyntax clangDependencyScanning clangDirectoryWatcher clangHandleLLVM LLVMMCJIT clangIncludeFixerPlugin clangIncludeFixer findAllSymbols clangIndexSerialization clangInterpreter LLVMOrcJIT LLVMExecutionEngine LLVMRuntimeDyld LLVMJITLink LLVMOrcTargetProcess LLVMOrcShared clangFrontendTool clangARCMigrate clangCodeGen LLVMCoverage clangRewriteFrontend clangMove clangQuery LLVMLineEditor clangDynamicASTMatchers clangReorderFields clangTesting clangTidyMain clangTidyPlugin clangTidyAbseilModule clangTidyAlteraModule clangTidyAndroidModule clangTidyBoostModule clangTidyCERTModule clangTidyConcurrencyModule clangTidyDarwinModule clangTidyFuchsiaModule clangTidyHICPPModule clangTidyBugproneModule clangTidyCppCoreGuidelinesModule clangTidyMiscModule clangTidyModernizeModule clangTidyGoogleModule clangTidyPerformanceModule clangTidyLLVMLibcModule clangTidyPortabilityModule clangTidyLLVMModule clangTidyReadabilityModule clangTidyLinuxKernelModule clangTidyMPIModule clangTidyObjCModule clangTidyOpenMPModule clangTidyZirconModule clangTidyUtils clangTidy clangStaticAnalyzerFrontend clangStaticAnalyzerCheckers clangStaticAnalyzerCore clangCrossTU clangTooling clangTransformer clangToolingRefactoring clangIndex clangFrontend clangDriver clangParse clangSerialization clangSema clangAnalysis clangASTMatchers clangEdit clangAST clangFormat clangToolingInclusions clangToolingCore clangRewrite clangLex clangBasic clangdRemoteIndex clangdSupport lldELF lldMachO lldMinGW lldCOFF LLVMDebugInfoPDB LLVMLibDriver LLVMWindowsManifest lldWasm LLVMAArch64AsmParser LLVMAArch64CodeGen LLVMAMDGPUAsmParser LLVMAMDGPUCodeGen LLVMMIRParser LLVMARMAsmParser LLVMARMCodeGen LLVMAVRAsmParser LLVMAVRCodeGen LLVMAVRDesc LLVMBPFAsmParser LLVMBPFCodeGen LLVMBPFDesc LLVMHexagonCodeGen LLVMHexagonAsmParser LLVMLanaiCodeGen LLVMLanaiAsmParser LLVMMSP430AsmParser LLVMMSP430CodeGen LLVMMSP430Desc LLVMMipsAsmParser LLVMMipsCodeGen LLVMMipsDesc LLVMNVPTXCodeGen LLVMNVPTXDesc LLVMNVPTXInfo LLVMPowerPCAsmParser LLVMPowerPCCodeGen LLVMPowerPCDesc LLVMRISCVAsmParser LLVMRISCVCodeGen LLVMSparcAsmParser LLVMSparcCodeGen LLVMSparcDesc LLVMSystemZAsmParser LLVMSystemZCodeGen LLVMVEAsmParser LLVMVECodeGen LLVMVEDesc LLVMWebAssemblyAsmParser LLVMWebAssemblyCodeGen LLVMX86AsmParser LLVMX86CodeGen LLVMCFGuard LLVMGlobalISel LLVMX86Desc LLVMXCoreCodeGen LLVMAsmPrinter LLVMDebugInfoMSF LLVMSelectionDAG LLVMXCoreDesc LLVMAArch64Disassembler LLVMAArch64Desc LLVMAArch64Info LLVMAArch64Utils LLVMAMDGPUDisassembler LLVMAMDGPUDesc LLVMAMDGPUInfo LLVMAMDGPUUtils LLVMARMDisassembler LLVMARMDesc LLVMARMInfo LLVMARMUtils LLVMAVRDisassembler LLVMAVRInfo LLVMBPFDisassembler LLVMBPFInfo LLVMMipsDisassembler LLVMMipsInfo LLVMPowerPCDisassembler LLVMPowerPCInfo LLVMX86Disassembler LLVMX86Info LLVMHexagonDisassembler LLVMHexagonDesc LLVMHexagonInfo LLVMLTO LLVMExtensions Polly LLVMPasses LLVMCoroutines LLVMipo LLVMFrontendOpenMP LLVMIRReader LLVMAsmParser LLVMInstrumentation LLVMLinker LLVMVectorize LLVMObjCARCOpts PollyISL LLVMLanaiDisassembler LLVMLanaiDesc LLVMLanaiInfo LLVMMSP430Disassembler LLVMMSP430Info LLVMRISCVDisassembler LLVMRISCVDesc LLVMRISCVInfo LLVMSparcDisassembler LLVMSparcInfo LLVMSystemZDisassembler LLVMSystemZDesc LLVMSystemZInfo LLVMVEDisassembler LLVMVEInfo LLVMWebAssemblyDisassembler LLVMWebAssemblyDesc LLVMWebAssemblyInfo LLVMWebAssemblyUtils LLVMXCoreDisassembler LLVMMCDisassembler LLVMXCoreInfo lldCommon LLVMCodeGen LLVMBitWriter LLVMScalarOpts LLVMAggressiveInstCombine LLVMInstCombine LLVMTransformUtils LLVMTarget LLVMAnalysis LLVMProfileData LLVMDebugInfoDWARF LLVMObject LLVMMCParser LLVMMC LLVMDebugInfoCodeView LLVMBitReader LLVMCore LLVMRemarks LLVMBitstreamReader LLVMTextAPI LLVMBinaryFormat LLVMOption LLVMSupport LLVMDemangle)

json="$(realpath "$json")"

get() {
    jq ".$1" "$json" | grep -Po "(?<=\")[^\"]+(?=\")" | tr '\n' ' ' || true # no dependency -> grep will fail
}

resolved=()
process() {
    lib="$1"
    if ! grep -Eq "(^|\s)$lib(\s|$)" <<< "${resolved[*]}"; then
        echo "checking $lib"
        resolved+=("$lib")
        # shellcheck disable=SC2207
        deps=( $(get "$lib") )
        for dep in "${deps[@]}"; do
            echo "$lib -> $dep"
            if grep -qE '^(LLVM|clang|lld)' <<< "$dep"; then
                process "$dep"
            else
                resolved+=("$dep")
            fi
        done
    fi
}

for lib in "${llvm[@]}"; do 
    process "$lib"
done

ordered_resolved=()
for lib in "${order[@]}"; do
    if grep -Eq "(^|\s)$lib(\s|$)" <<< "${resolved[*]}"; then
        ordered_resolved+=("$lib")
    fi
done
# get missed libs e.g. system libs
for lib in "${resolved[@]}"; do
    if ! grep -Eq "(^|\s)$lib(\s|$)" <<< "${ordered_resolved[*]}"; then
        ordered_resolved+=("$lib")
    fi
done
printf "%s\n" "${ordered_resolved[@]}" > resolved.log

echo -e "\ncreated resolved.log in your current working directory!"
