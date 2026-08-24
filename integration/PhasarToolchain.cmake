# PhasarToolchain.cmake — configure a build tree that emits whole-program LLVM IR.
#
#   cmake -S . -B build-phasar --toolchain <phasar>/integration/PhasarToolchain.cmake
#   cmake --build build-phasar          # extraction is part of ALL
#   -> build-phasar/phasar-ir/whole-program.bc, ready for phasar-cli -m
#
# Prerequisites: CMake >= 3.21, clang, wllvm (pipx install wllvm), the `file`
# utility (wllvm shells out to it), llvm-link and llvm-ar from the matching
# llvm-<N> package (clang-<N> alone does not ship them), POSIX host.
#
# Options:
#   CMAKE_C_COMPILER             the clang to wrap; CMAKE_CXX_COMPILER is used
#                                for C++-only projects (else the first clang found)
#   PHASAR_IR_BITCODE_FLAGS      clang flags for the preserved bitcode
#   PHASAR_IR_TARGET             target to extract; default: the sole executable
#   PHASAR_IR_LLVM_VERSION       LLVM major version phasar-cli understands (22)
#   PHASAR_IR_CLANG_VERSIONS     accepted clang major versions (16;...;22)
#   PHASAR_IR_WLLVM_BIN_DIR      directory holding wllvm/wllvm++/extract-bc
#   PHASAR_IR_CHAINLOAD_TOOLCHAIN_FILE  an existing toolchain to load first

# One shape for every diagnostic: what is the case, why it is a problem, how to
# move on — details last, so the instruction is not buried in a list.
# Messages name the toolchain file, not the phasar-ir build target: in another
# project's output it has to be recognisable which component is speaking.
function(_phasar_ir_diagnose level what why how details)
  # CMake renders every line as its own paragraph, so an extra blank line would
  # only show up as an indented line of spaces.
  set(text "PhasarToolchain.cmake\nWhat: ${what}\nWhy: ${why}\nFix: ${how}")
  if(details)
    string(APPEND text "\nDetails: ${details}")
  endif()
  message(${level} "${text}")
endfunction()

if(CMAKE_VERSION VERSION_LESS 3.21)
  _phasar_ir_diagnose(FATAL_ERROR
      "CMake ${CMAKE_VERSION} is too old, 3.21 or newer is required"
      "the extraction step needs cmake_language(DEFER) from 3.19, and the documented --toolchain option only exists from 3.21"
      "upgrade CMake, or produce the bitcode with utils/phasar-analyze.sh instead" "")
endif()

if(PHASAR_IR_CHAINLOAD_TOOLCHAIN_FILE)
  include("${PHASAR_IR_CHAINLOAD_TOOLCHAIN_FILE}")
endif()

# Both guards are load-bearing. try_compile() re-reads this file, but neither our
# cache variables nor our global properties reach that sub-configure, so the
# setup below would re-run discovery with none of its inputs and abort every
# compiler probe. Outside try_compile the file is still re-read once per enabled
# language. Only the assignments at the bottom may run on every read.
get_property(_phasar_ir_in_try_compile GLOBAL PROPERTY IN_TRY_COMPILE)
get_property(_phasar_ir_setup_done GLOBAL PROPERTY _PHASAR_IR_SETUP_DONE)

# Locates the tools, checks their versions, generates the compiler wrappers and
# wires up the extraction target. Runs exactly once per configure.
function(_phasar_ir_setup)

  if(WIN32)
    _phasar_ir_diagnose(FATAL_ERROR
      "this toolchain does not support Windows"
      "wllvm marks each object with an ELF section that extract-bc follows after linking"
      "use a POSIX host" "")
  endif()

  # PhASAR's own default is 16 (CMakeLists.txt, BUILD.md). A different value here
  # would be a second, silently diverging contract: phasar-cli would reject the
  # module only at the very end, with no warning from us.
  set(PHASAR_IR_LLVM_VERSION "16" CACHE STRING
      "LLVM major version the target phasar-cli was built against")
  set(PHASAR_IR_CLANG_VERSIONS "16;17;18;19;20;21;22" CACHE STRING
      "Accepted clang major versions")
  set(PHASAR_IR_TARGET "" CACHE STRING
      "Target to extract bitcode from; empty means the project's sole executable")
  # Decides which IR is analysed, and therefore the analysis results — hence a
  # visible option. -O0 with disable-O0-optnone keeps the IR close to the source;
  # "-g -O1 -Xclang -disable-llvm-optzns" is the alternative when you want
  # cleaned-up IR without optimisation passes.
  set(PHASAR_IR_BITCODE_FLAGS "-g -O0 -Xclang -disable-O0-optnone" CACHE STRING
      "clang flags for the bitcode wllvm preserves")

  # wllvm
  if(NOT PHASAR_IR_WLLVM_BIN_DIR)
    find_program(_wllvm wllvm NO_CACHE)
    if(NOT _wllvm)
      _phasar_ir_diagnose(FATAL_ERROR
      "wllvm was not found on PATH"
      "wllvm compiles through clang while preserving the bitcode of every translation unit"
      "run 'pipx install wllvm', or point at it with -DPHASAR_IR_WLLVM_BIN_DIR=<dir>" "")
    endif()
    get_filename_component(_dir "${_wllvm}" DIRECTORY)
    set(PHASAR_IR_WLLVM_BIN_DIR "${_dir}" CACHE PATH "Directory holding wllvm" FORCE)
  endif()

  # Without `file`, wllvm still writes per-TU bitcode but never stamps the
  # .llvm_bc section, so extract-bc silently finds nothing later.
  find_program(_file file NO_CACHE)
  if(NOT _file)
    _phasar_ir_diagnose(FATAL_ERROR
      "the 'file' utility is missing"
      "without it wllvm writes the bitcode but never marks the objects, so extract-bc silently finds nothing"
      "install the 'file' package" "")
  endif()

  foreach(_prog wllvm wllvm++ extract-bc)
    if(NOT EXISTS "${PHASAR_IR_WLLVM_BIN_DIR}/${_prog}")
      _phasar_ir_diagnose(FATAL_ERROR
      "no wllvm in ${PHASAR_IR_WLLVM_BIN_DIR}: ${_prog} is missing"
      "wllvm, wllvm++ and extract-bc are all required and ship together"
      "reinstall wllvm, or correct -DPHASAR_IR_WLLVM_BIN_DIR" "")
    endif()
  endforeach()
  set(PHASAR_IR_EXTRACT_BC "${PHASAR_IR_WLLVM_BIN_DIR}/extract-bc"
      CACHE FILEPATH "extract-bc used by phasar-ir" FORCE)

  # Major version of a candidate compiler; empty if it is not clang at all.
  function(_phasar_ir_clang_major exe out_var)
    execute_process(COMMAND "${exe}" --version
                    OUTPUT_VARIABLE out ERROR_VARIABLE out RESULT_VARIABLE rc)
    if(rc EQUAL 0 AND out MATCHES "clang version ([0-9]+)")
      set(${out_var} "${CMAKE_MATCH_1}" PARENT_SCOPE)
    else()
      set(${out_var} "" PARENT_SCOPE)
    endif()
  endfunction()

  # Several clang installations usually coexist, so enumerate candidates instead
  # of taking the first `clang` on PATH. Preference: the version phasar-cli
  # understands, then older accepted ones (bitcode is backward compatible), then
  # newer ones as a last resort. Versioned names are probed inside per-version
  # install dirs too, since those are often not on PATH.
  # Candidate order: the version phasar-cli understands, then older accepted ones
  # (bitcode is backward compatible), newer ones last.
  function(_phasar_ir_clang_search_order out_var)
    set(older "")
    set(newer "")
    foreach(v IN LISTS PHASAR_IR_CLANG_VERSIONS)
      if(v VERSION_EQUAL PHASAR_IR_LLVM_VERSION)
        # already first
      elseif(v VERSION_LESS PHASAR_IR_LLVM_VERSION)
        list(APPEND older "${v}")
      else()
        list(APPEND newer "${v}")
      endif()
    endforeach()
    list(SORT older COMPARE NATURAL ORDER DESCENDING)
    list(SORT newer COMPARE NATURAL)
    set(${out_var} "${PHASAR_IR_LLVM_VERSION}" ${older} ${newer} PARENT_SCOPE)
  endfunction()

  # Looks for a clang of the requested version. PATHS rather than HINTS: whatever
  # is on PATH wins, the per-version install directories are the fallback.
  function(_phasar_ir_clang_candidate version out_var)
    unset(cand)
    if(version STREQUAL "any")
      find_program(cand NAMES clang NO_CACHE)
    else()
      find_program(cand NAMES "clang-${version}"
                   PATHS "/usr/lib/llvm-${version}/bin"
                         "/usr/local/opt/llvm@${version}/bin"
                         "/opt/homebrew/opt/llvm@${version}/bin"
                   NO_CACHE)
    endif()
    set(${out_var} "${cand}" PARENT_SCOPE)
  endfunction()

  # Several clang installations are the normal case, so candidates are enumerated
  # instead of taking the first one on PATH. What is checked is the reported
  # version, not the file name.
  function(_phasar_ir_find_clang out_var)
    _phasar_ir_clang_search_order(ordered)
    set(rejected "")

    foreach(v IN LISTS ordered "any")
      _phasar_ir_clang_candidate("${v}" cand)
      if(cand)
        _phasar_ir_clang_major("${cand}" m)
        if(m AND m IN_LIST PHASAR_IR_CLANG_VERSIONS)
          set(${out_var} "${cand}" PARENT_SCOPE)
          return()
        endif()
        list(APPEND rejected "${cand} (clang ${m})")
      endif()
    endforeach()

    if(rejected)
      string(REPLACE ";" ", " seen "${rejected}")
      _phasar_ir_diagnose(FATAL_ERROR
        "no clang in PHASAR_IR_CLANG_VERSIONS (${PHASAR_IR_CLANG_VERSIONS})"
        "every clang found reports a version outside that list, so its bitcode is not declared as working with your phasar-cli"
        "install an accepted clang, or widen -DPHASAR_IR_CLANG_VERSIONS once you have verified that version" "${seen}")
    endif()
    _phasar_ir_diagnose(FATAL_ERROR
      "no clang was found"
      "PhASAR analyses LLVM IR, which only an LLVM frontend can produce"
      "install clang ${PHASAR_IR_LLVM_VERSION}, or select one with -DCMAKE_C_COMPILER=<clang>" "")
  endfunction()

  # What the user configured for a language, or empty. Precedence matches CMake's
  # own: CMAKE_<LANG>_COMPILER wins over the environment variable.
  function(_phasar_ir_configured_compiler lang out_var)
    if(lang STREQUAL "C")
      set(configured "${CMAKE_C_COMPILER}")
      set(from_env "$ENV{CC}")
    else()
      set(configured "${CMAKE_CXX_COMPILER}")
      set(from_env "$ENV{CXX}")
    endif()
    if(configured)
      set(${out_var} "${configured}" PARENT_SCOPE)
    else()
      set(${out_var} "${from_env}" PARENT_SCOPE)
    endif()
  endfunction()

  # Cached because we overwrite CMAKE_C_COMPILER below, so on a reconfigure it
  # would otherwise point back at wllvm.
  if(NOT PHASAR_IR_CLANG)
    _phasar_ir_configured_compiler(C _configured_c)
    _phasar_ir_configured_compiler(CXX _configured_cxx)
    # A C++-only project selects its compiler through CMAKE_CXX_COMPILER; without
    # this branch a g++ would be accepted silently there.
    if(_configured_c)
      set(_clang "${_configured_c}")
    elseif(_configured_cxx)
      set(_clang "${_configured_cxx}")
    else()
      _phasar_ir_find_clang(_clang)
    endif()
    # CMAKE_C_COMPILER and $ENV{CC} may be bare names ("clang"). Resolve them,
    # otherwise the install directory and version suffix cannot be derived.
    if(NOT IS_ABSOLUTE "${_clang}")
      get_filename_component(_resolved "${_clang}" PROGRAM)
      if(_resolved)
        set(_clang "${_resolved}")
      endif()
    endif()
    set(PHASAR_IR_CLANG "${_clang}" CACHE FILEPATH "clang wrapped by phasar-ir" FORCE)
  endif()

  # Re-checked even when found by search, so an explicitly configured compiler
  # gets the same validation.
  _phasar_ir_clang_major("${PHASAR_IR_CLANG}" _major)
  if(NOT _major)
    _phasar_ir_diagnose(FATAL_ERROR
      "'${PHASAR_IR_CLANG}' is not clang"
      "PhASAR analyses LLVM IR, which gcc and MSVC cannot emit"
      "set -DCMAKE_C_COMPILER (or CC) to a clang" "")
  endif()

  # If both languages are configured, both must be clang and of the same major
  # version — otherwise the module mixes bitcode from two frontends.
  if(_configured_c AND _configured_cxx)
    _phasar_ir_clang_major("${_configured_cxx}" _cxx_major)
    if(NOT _cxx_major)
      _phasar_ir_diagnose(FATAL_ERROR
      "'${_configured_cxx}' is not clang"
      "the C++ sources would be compiled by a frontend that emits no LLVM IR"
      "set -DCMAKE_CXX_COMPILER (or CXX) to a clang++" "")
    endif()
    if(NOT _cxx_major EQUAL _major)
      _phasar_ir_diagnose(FATAL_ERROR
      "clang ${_major} and clang++ ${_cxx_major} differ"
      "the module would mix bitcode from two frontend versions"
      "use one clang version for both languages" "${PHASAR_IR_CLANG} / ${_configured_cxx}")
    endif()
  endif()
  if(NOT _major IN_LIST PHASAR_IR_CLANG_VERSIONS)
    _phasar_ir_diagnose(FATAL_ERROR
      "clang ${_major} not in PHASAR_IR_CLANG_VERSIONS (${PHASAR_IR_CLANG_VERSIONS})"
      "that version is not declared as verified against your phasar-cli"
      "choose an accepted clang, or widen -DPHASAR_IR_CLANG_VERSIONS after verifying it" "${PHASAR_IR_CLANG}")
  endif()
  if(_major GREATER PHASAR_IR_LLVM_VERSION)
    _phasar_ir_diagnose(WARNING
      "clang ${_major} is newer than phasar-cli's LLVM ${PHASAR_IR_LLVM_VERSION}"
      "LLVM reads older bitcode but not newer, so phasar-cli will most likely reject the module"
      "set -DPHASAR_IR_LLVM_VERSION to the version your phasar-cli was built with, or use a matching clang" "")
  endif()

  # Derive the version suffix from the binary name; the driver names below cover
  # both directions, because the selected compiler may be the C++ driver.
  get_filename_component(_driver_name "${PHASAR_IR_CLANG}" NAME)
  set(_suffix "")
  if(_driver_name MATCHES "-([0-9]+)$")
    set(_suffix "-${CMAKE_MATCH_1}")
  endif()
  get_filename_component(_bin_dir "${PHASAR_IR_CLANG}" DIRECTORY)

  # extract-bc shells out to llvm-link (and llvm-ar for archives). Look them up
  # by detected major version as well as by the name suffix, because an
  # unsuffixed /usr/bin/clang can sit next to a suffixed llvm-link-<N>. Both are
  # then handed to extract-bc explicitly.
  find_program(PHASAR_IR_LLVM_LINK
               NAMES "llvm-link-${_major}" "llvm-link${_suffix}" llvm-link
               PATHS "${_bin_dir}" "/usr/lib/llvm-${_major}/bin" NO_CACHE)
  if(NOT PHASAR_IR_LLVM_LINK)
    _phasar_ir_diagnose(FATAL_ERROR
      "llvm-link was not found"
      "extract-bc links the preserved per-file bitcode into one module with it"
      "install the llvm-${_major} package — clang-${_major} alone does not ship it" "")
  endif()
  find_program(PHASAR_IR_LLVM_AR
               NAMES "llvm-ar-${_major}" "llvm-ar${_suffix}" llvm-ar
               PATHS "${_bin_dir}" "/usr/lib/llvm-${_major}/bin" NO_CACHE)
  if(NOT PHASAR_IR_LLVM_AR)
    _phasar_ir_diagnose(FATAL_ERROR
      "llvm-ar was not found"
      "extract-bc needs it to read the bitcode inside static archives"
      "install the llvm-${_major} package — clang-${_major} alone does not ship it" "")
  endif()
  set(PHASAR_IR_EXTRACT_BC_ARGS
      -l "${PHASAR_IR_LLVM_LINK}" -a "${PHASAR_IR_LLVM_AR}"
      CACHE STRING "Tool paths handed to extract-bc" FORCE)

  # wllvm is configured exclusively through environment variables, which CMake
  # cannot set for build steps. A compiler launcher would be the shorter route but
  # fails twice over: CMake runs the compiler WITHOUT launchers when identifying
  # it (result: an empty CMAKE_<LANG>_COMPILER_ID and target_compile_features
  # aborting), and the launcher variable belongs to the project, which commonly
  # uses it for ccache or overrides it per target. The environment therefore
  # belongs inside the compiler itself.
  set(_wrapper_dir "${CMAKE_BINARY_DIR}/phasar-ir")
  file(MAKE_DIRECTORY "${_wrapper_dir}")

  # The wrappers are named after the real drivers, not after us. CMake derives a
  # toolchain suffix from the compiler's base name (CMakeDetermineCXXCompiler),
  # and a name like "phasar-cxx" matches nothing — CMAKE_AR, CMAKE_NM,
  # CMAKE_OBJDUMP and friends then fall back to GNU binutils instead of the
  # matching LLVM ones, or end up NOTFOUND.
  foreach(_lang C CXX)
    if(_lang STREQUAL "C")
      set(_wrapper "${_wrapper_dir}/clang-${_major}")
      set(_tool wllvm)
      set(_name_var LLVM_CC_NAME)
      set(_driver "clang${_suffix}")
    else()
      set(_wrapper "${_wrapper_dir}/clang++-${_major}")
      set(_tool wllvm++)
      set(_name_var LLVM_CXX_NAME)
      set(_driver "clang++${_suffix}")
    endif()
    file(WRITE "${_wrapper}"
"#!/bin/sh
# Generated by PhasarToolchain.cmake — do not edit.
set -e
export LLVM_COMPILER=clang
export LLVM_COMPILER_PATH='${_bin_dir}'
export ${_name_var}='${_driver}'
export LLVM_BITCODE_GENERATION_FLAGS=\"${PHASAR_IR_BITCODE_FLAGS} \${LLVM_BITCODE_GENERATION_FLAGS:-}\"
exec '${PHASAR_IR_WLLVM_BIN_DIR}/${_tool}' \"$@\"
")
    file(CHMOD "${_wrapper}" PERMISSIONS
         OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE
         WORLD_READ WORLD_EXECUTE)
  endforeach()

  set(PHASAR_IR_CC "${_wrapper_dir}/clang-${_major}" CACHE FILEPATH
      "Generated C compiler wrapping wllvm" FORCE)
  set(PHASAR_IR_CXX "${_wrapper_dir}/clang++-${_major}" CACHE FILEPATH
      "Generated C++ compiler wrapping wllvm" FORCE)

  # C++20 modules need clang-scan-deps, whose path CMake derives from the
  # compiler directory (Compiler/Clang-FindBinUtils.cmake). Next to a generated
  # wrapper there is none, and on Debian the tool often only exists inside
  # /usr/lib/llvm-<N>/bin, which is not on PATH — so point at it explicitly.
  find_program(PHASAR_IR_SCAN_DEPS
               NAMES "clang-scan-deps-${_major}" clang-scan-deps
               PATHS "${_bin_dir}" "/usr/lib/llvm-${_major}/bin" NO_CACHE)
  if(PHASAR_IR_SCAN_DEPS)
    set(PHASAR_IR_SCAN_DEPS "${PHASAR_IR_SCAN_DEPS}" CACHE FILEPATH
        "clang-scan-deps used for C++20 modules" FORCE)
  endif()

  message(STATUS "PhasarToolchain: clang ${_major} via wllvm (${PHASAR_IR_WLLVM_BIN_DIR})")

  # Despite being registered early this runs at the end of the top-level scope,
  # after every add_subdirectory() — so the targets already exist, including on
  # the first configure of a fresh build tree (test cases 01-08 prove it).
  function(_phasar_ir_collect_targets dir out_var)
    get_property(targets DIRECTORY "${dir}" PROPERTY BUILDSYSTEM_TARGETS)
    get_property(subdirs DIRECTORY "${dir}" PROPERTY SUBDIRECTORIES)
    foreach(sub IN LISTS subdirs)
      _phasar_ir_collect_targets("${sub}" sub_targets)
      list(APPEND targets ${sub_targets})
    endforeach()
    set(${out_var} "${targets}" PARENT_SCOPE)
  endfunction()

  # Module file sets exist from CMake 3.28; on older versions the property is
  # simply not set and nothing is reported, which is correct because those
  # versions cannot build modules either.
  function(_phasar_ir_warn_about_module_sources tgt)
    get_target_property(sets ${tgt} CXX_MODULE_SETS)
    if(NOT sets)
      return()
    endif()
    set(units "")
    foreach(set_name IN LISTS sets)
      get_target_property(files ${tgt} CXX_MODULE_SET_${set_name})
      if(files)
        list(APPEND units ${files})
      endif()
    endforeach()
    if(NOT units)
      return()
    endif()
    list(TRANSFORM units REPLACE "^${CMAKE_SOURCE_DIR}/" "")
    string(REPLACE ";" ", " unit_list "${units}")
    _phasar_ir_diagnose(WARNING
      "target '${tgt}' has C++20 module units, whose code will be missing from the module"
      "wllvm decides what is a source by file extension and does not list .cppm, so those objects never carry bitcode and extract-bc cannot find it"
      "facade modules that only re-export names lose nothing; for module units with real code, add cppm|ixx|cxxm|ccm to wllvm's extension regex in arglistfilter.py"
      "${unit_list}")
  endfunction()

  # One module for the whole project. With several executables there is no single
  # whole-program module, so the target must be named explicitly.
  function(_phasar_ir_finalize)
    if(PHASAR_IR_TARGET)
      if(NOT TARGET "${PHASAR_IR_TARGET}")
        _phasar_ir_diagnose(FATAL_ERROR
      "PHASAR_IR_TARGET '${PHASAR_IR_TARGET}' is not a target"
      "the bitcode is extracted from one built artifact, which has to exist in this project"
      "use a name as spelled in add_executable() or add_library()" "")
      endif()
      set(chosen "${PHASAR_IR_TARGET}")
    else()
      _phasar_ir_collect_targets("${CMAKE_CURRENT_SOURCE_DIR}" all_targets)
      set(exes "")
      foreach(tgt IN LISTS all_targets)
        get_target_property(type ${tgt} TYPE)
        if(type STREQUAL "EXECUTABLE")
          list(APPEND exes "${tgt}")
        endif()
      endforeach()
      list(LENGTH exes n)
      if(n EQUAL 0)
        _phasar_ir_diagnose(FATAL_ERROR
      "no executable target in this project"
      "without an executable nothing is linked, so no whole-program module can be produced"
      "name a library instead, e.g. -DPHASAR_IR_TARGET=<lib>, and analyse it with -E __ALL__" "")
      elseif(n GREATER 1)
        string(REPLACE ";" ", " candidates "${exes}")
        _phasar_ir_diagnose(FATAL_ERROR
      "several executables found (${n})"
      "one build tree yields exactly one module, and several entry points have no common whole-program module"
      "pick one with -DPHASAR_IR_TARGET=<target>; analysing several means one build tree each" "${candidates}")
      endif()
      set(chosen "${exes}")
    endif()

    # We know one thing gets left out silently, so say it. wllvm decides what is a
    # source by extension and does not list .cppm, so module units never carry
    # bitcode and their code is missing from the module without any sign of it.
    _phasar_ir_warn_about_module_sources("${chosen}")

    get_target_property(type ${chosen} TYPE)
    set(flag "")
    if(type STREQUAL "STATIC_LIBRARY")
      set(flag "-b")  # bitcode-archive mode
    elseif(NOT type MATCHES "^(EXECUTABLE|SHARED_LIBRARY|MODULE_LIBRARY)$")
      _phasar_ir_diagnose(FATAL_ERROR
      "target '${chosen}' is a ${type}, so there is nothing to extract"
      "only executables, shared libraries and static archives carry compiled objects"
      "name an executable or a library target" "")
    endif()

    # Produces the module only. Invoking phasar-cli deliberately does not belong
    # here: analyses run for an unbounded time and the user picks them.
    # Multi-config generators build several configurations into the same build
    # tree; without the configuration name in the path the last build overwrites
    # the previous module, and you unknowingly analyse Release instead of Debug.
    get_property(_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
    if(_multi_config)
      set(module_dir "${CMAKE_BINARY_DIR}/phasar-ir/$<CONFIG>")
    else()
      set(module_dir "${CMAKE_BINARY_DIR}/phasar-ir")
    endif()
    set(module "${module_dir}/whole-program.bc")

    add_custom_command(
      OUTPUT "${module}"
      COMMAND "${CMAKE_COMMAND}" -E make_directory "${module_dir}"
      COMMAND "${PHASAR_IR_EXTRACT_BC}" ${PHASAR_IR_EXTRACT_BC_ARGS} ${flag}
              "$<TARGET_FILE:${chosen}>" -o "${module}"
      # extract-bc exits 0 even when it produces nothing, so check explicitly.
      COMMAND test -s "${module}"
      DEPENDS ${chosen}
      COMMENT "phasar-ir: extracting whole-program bitcode from ${chosen}"
      VERBATIM)
    # Part of ALL: this build tree exists only to produce IR, and extraction is
    # just an llvm-link of bitcode that already exists.
    add_custom_target(phasar-ir ALL DEPENDS "${module}")
  endfunction()

  # Runs at the end of the top-level scope, once every target exists.
  cmake_language(DEFER CALL _phasar_ir_finalize)
endfunction()

if(NOT _phasar_ir_in_try_compile AND NOT _phasar_ir_setup_done)
  set_property(GLOBAL PROPERTY _PHASAR_IR_SETUP_DONE TRUE)
  _phasar_ir_setup()
endif()

# Applied on every read of this file, including inside try_compile(), where the
# values arrive from the parent configure.
if(PHASAR_IR_CC)
  set(CMAKE_C_COMPILER "${PHASAR_IR_CC}")
endif()
if(PHASAR_IR_CXX)
  set(CMAKE_CXX_COMPILER "${PHASAR_IR_CXX}")
endif()
# CXX only: the module rule that consumes this is C++-specific.
if(PHASAR_IR_SCAN_DEPS)
  set(CMAKE_CXX_COMPILER_CLANG_SCAN_DEPS "${PHASAR_IR_SCAN_DEPS}")
endif()
