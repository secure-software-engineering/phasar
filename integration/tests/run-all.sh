#!/bin/bash
# run-all.sh — run every test case against ../PhasarToolchain.cmake.
#
# A case is a file <project>/cases/<name>.case declaring what must hold; see
# cases-TEMPLATE.case for the fields. This driver discovers the cases, validates
# all of them before running anything, and derives from the declared fields what
# to do: module assertions mean build and disassemble, a message assertion means
# configure and inspect the diagnostic, a selection assertion means read the
# cache. In every case the exit status and the shape of our own diagnostics are
# checked as well.
#
# Requires: cmake >= 3.19, clang, wllvm, file, llvm-link (llvm-dis optional).

set -euo pipefail
shopt -s extglob

HERE="$(cd "$(dirname "$0")" && pwd)"
TOOLCHAIN="$HERE/../PhasarToolchain.cmake"
EXTRA_ARGS=("$@")

PASSED=0
FAILED=0

main() {
    check_prerequisites
    GENERATOR_DEFAULT="$(pick_generator)"
    DISASSEMBLER="$(find_disassembler)"
    printf 'generator: %s\ndisasm   : %s\n\n' "$GENERATOR_DEFAULT" "$DISASSEMBLER"

    mapfile -t cases < <(discover_cases)
    if [ "${#cases[@]}" -eq 0 ]; then
        die "no case files${PHASAR_TEST_CASE:+ matching '$PHASAR_TEST_CASE'} found"
    fi

    validate_all "${cases[@]}"

    local case_file
    for case_file in "${cases[@]}"; do
        run_case "$case_file"
    done

    printf '\npassed: %s, failed: %s\n' "$PASSED" "$FAILED"
    [ "$FAILED" -eq 0 ]
}

die() { printf 'run-all.sh: %s\n' "$1" >&2; exit 1; }

# ---------------------------------------------------------------------------
# environment
# ---------------------------------------------------------------------------

# Report every missing tool once, instead of letting each case fail separately.
check_prerequisites() {
    local missing=()
    command -v cmake >/dev/null 2>&1 || missing+=("cmake (>= 3.19)")
    command -v wllvm >/dev/null 2>&1 || missing+=("wllvm (pipx install wllvm)")
    command -v file  >/dev/null 2>&1 || missing+=("file")
    have_clang     || missing+=("clang")
    have_llvm_link || missing+=("llvm-link (package llvm-<N>)")
    if [ "${#missing[@]}" -gt 0 ]; then
        printf 'run-all.sh: missing prerequisites:\n' >&2
        printf '  - %s\n' "${missing[@]}" >&2
        exit 1
    fi
}

have_clang() {
    local v
    for v in 22 21 20 19 18 17 16; do
        command -v "clang-$v" >/dev/null 2>&1 && return 0
    done
    command -v clang >/dev/null 2>&1
}

have_llvm_link() {
    local v
    for v in 22 21 20 19 18 17 16; do
        command -v "llvm-link-$v" >/dev/null 2>&1 && return 0
        [ -x "/usr/lib/llvm-$v/bin/llvm-link" ] && return 0
    done
    command -v llvm-link >/dev/null 2>&1
}

pick_generator() {
    if command -v ninja >/dev/null 2>&1; then echo "Ninja"; else echo "Unix Makefiles"; fi
}

# llvm-dis must match the bitcode version; clang can read bitcode too and is a
# prerequisite anyway, so it stands in when llvm-dis is absent.
find_disassembler() {
    local v candidate
    for v in 22 21 20 19 18 17 16; do
        for candidate in "llvm-dis-$v" "/usr/lib/llvm-$v/bin/llvm-dis"; do
            command -v "$candidate" >/dev/null 2>&1 && { echo "$candidate"; return; }
        done
    done
    command -v llvm-dis >/dev/null 2>&1 && { echo "llvm-dis"; return; }
    for v in 22 21 20 19 18 17 16; do
        command -v "clang-$v" >/dev/null 2>&1 && { echo "clang-$v -S -emit-llvm"; return; }
    done
    echo "clang -S -emit-llvm"
}

# The first accepted clang on this host, in the same order the toolchain uses.
detected_clang_major() {
    local v
    for v in 22 21 20 19 18 17 16; do
        command -v "clang-$v" >/dev/null 2>&1 && { echo "$v"; return; }
    done
    clang --version | sed -n 's/.*clang version \([0-9]*\).*/\1/p' | head -1
}

# ${1-} rather than $1: under `set -u` a call without an argument would abort.
detected_clang() { printf 'clang%s-%s' "${1-}" "$(detected_clang_major)"; }

# ---------------------------------------------------------------------------
# cases
# ---------------------------------------------------------------------------

# <project>/<name> of a case file, which is also how it is reported.
case_id() {
    local file="$1" project name
    project="$(basename "$(dirname "$(dirname "$file")")")"
    name="$(basename "$file" .case)"
    printf '%s/%s' "$project" "$name"
}

# PHASAR_TEST_CASE keeps only cases whose id contains the substring, for
# iterating on one case; PHASAR_TEST_SKIP drops matching ones, which is how an
# environment excludes a case it cannot support. Both match the id, not the path.
discover_cases() {
    local file id
    while IFS= read -r file; do
        id="$(case_id "$file")"
        if [ -n "${PHASAR_TEST_CASE:-}" ]; then
            case "$id" in *"$PHASAR_TEST_CASE"*) ;; *) continue ;; esac
        fi
        if [ -n "${PHASAR_TEST_SKIP:-}" ]; then
            case "$id" in *"$PHASAR_TEST_SKIP"*) printf 'SKIP  %s\n' "$id" >&2; continue ;; esac
        fi
        printf '%s\n' "$file"
    done < <(find "$HERE"/*/cases -name '*.case' -type f | sort)
}

trim() { local s="$1"; s="${s#"${s%%[![:space:]]*}"}"; printf '%s' "${s%"${s##*[![:space:]]}"}"; }

SINGLE_KEYS="description cmake_generator expect_selected_clang expect_message_content expect_message_level"
LIST_KEYS="cmake_additional_arg cmake_configuration stubbed_clang expect_module_function forbid_module_function"

# Parses one case file into c_* variables and collects problems in PARSE_PROBLEMS.
parse_case() {
    local file="$1" line key value lineno=0
    c_description=""; c_cmake_generator=""; c_expect_selected_clang=""
    c_expect_message_content=""; c_expect_message_level=""
    c_cmake_additional_arg=(); c_cmake_configuration=(); c_stubbed_clang=()
    c_expect_module_function=(); c_forbid_module_function=()
    PARSE_PROBLEMS=()

    while IFS= read -r line || [ -n "$line" ]; do
        lineno=$((lineno + 1))
        case "$(trim "$line")" in ''|'#'*) continue ;; esac
        if [[ "$line" != *=* ]]; then
            PARSE_PROBLEMS+=("$file:$lineno: not a 'key = value' line")
            continue
        fi
        key="$(trim "${line%%=*}")"
        value="$(trim "${line#*=}")"
        if [ -z "$value" ]; then
            PARSE_PROBLEMS+=("$file:$lineno: empty value for '$key'")
            continue
        fi
        case " $SINGLE_KEYS " in
            *" $key "*)
                if [ -n "$(single_value "$key")" ]; then
                    PARSE_PROBLEMS+=("$file:$lineno: '$key' set twice")
                    continue
                fi
                set_single "$key" "$value"
                continue ;;
        esac
        case " $LIST_KEYS " in
            *" $key "*) append_list "$key" "$value"; continue ;;
        esac
        PARSE_PROBLEMS+=("$file:$lineno: unknown key '$key'")
    done < "$file"
}

single_value() {
    case "$1" in
        description)            printf '%s' "$c_description" ;;
        cmake_generator)        printf '%s' "$c_cmake_generator" ;;
        expect_selected_clang)  printf '%s' "$c_expect_selected_clang" ;;
        expect_message_content) printf '%s' "$c_expect_message_content" ;;
        expect_message_level)   printf '%s' "$c_expect_message_level" ;;
    esac
}

set_single() {
    case "$1" in
        description)            c_description="$2" ;;
        cmake_generator)        c_cmake_generator="$2" ;;
        expect_selected_clang)  c_expect_selected_clang="$2" ;;
        expect_message_content) c_expect_message_content="$2" ;;
        expect_message_level)   c_expect_message_level="$2" ;;
    esac
}

append_list() {
    case "$1" in
        cmake_additional_arg)   c_cmake_additional_arg+=("$2") ;;
        cmake_configuration)    c_cmake_configuration+=("$2") ;;
        stubbed_clang)          c_stubbed_clang+=("$2") ;;
        expect_module_function) c_expect_module_function+=("$2") ;;
        forbid_module_function) c_forbid_module_function+=("$2") ;;
    esac
}

# All case files are validated before the first one runs, so a typo is reported
# with every other finding instead of at case seventeen.
validate_all() {
    local file problems=()
    for file in "$@"; do
        parse_case "$file"
        problems+=(${PARSE_PROBLEMS[@]+"${PARSE_PROBLEMS[@]}"})
        [ -n "$c_description" ] || problems+=("$file: no description")
        if [ "${#c_expect_module_function[@]}" -eq 0 ] \
           && [ -z "$c_expect_message_content" ] \
           && [ -z "$c_expect_selected_clang" ]; then
            problems+=("$file: no positive assertion (expect_module_function, expect_message_content or expect_selected_clang)")
        fi
        if [ -n "$c_expect_message_content" ] && [ -z "$c_expect_message_level" ]; then
            problems+=("$file: expect_message_content without expect_message_level")
        fi
        if [ -n "$c_expect_message_level" ]; then
            [ -n "$c_expect_message_content" ] \
                || problems+=("$file: expect_message_level without expect_message_content")
            case "$c_expect_message_level" in
                error|warning|status) ;;
                *) problems+=("$file: expect_message_level must be error, warning or status") ;;
            esac
        fi
        local entry
        for entry in ${c_stubbed_clang[@]+"${c_stubbed_clang[@]}"}; do
            if [[ "$entry" != ?*=+([0-9]) ]]; then
                problems+=("$file: stubbed_clang '$entry' is not <name>=<major version>")
            fi
        done
        if [ "${#c_cmake_configuration[@]}" -gt 0 ] \
           && [[ "$c_cmake_generator" != *Multi-Config* ]]; then
            problems+=("$file: cmake_configuration needs a multi-config cmake_generator")
        fi
    done
    if [ "${#problems[@]}" -gt 0 ]; then
        printf 'run-all.sh: invalid case files:\n' >&2
        printf '  - %s\n' "${problems[@]}" >&2
        exit 1
    fi
}

# ---------------------------------------------------------------------------
# running one case
# ---------------------------------------------------------------------------

run_case() {
    local file="$1" project name build stubs generator output status
    project="$(basename "$(dirname "$(dirname "$file")")")"
    name="$(basename "$file" .case)"
    # PARSE_PROBLEMS is not inspected here: validate_all has already aborted the
    # run for any file that produced one.
    parse_case "$file"

    build="$HERE/$project/build/$name"
    rm -rf "$build"
    stubs=""
    if [ "${#c_stubbed_clang[@]}" -gt 0 ]; then
        stubs="$(mktemp -d)"
        write_stubs "$stubs" "${c_stubbed_clang[@]}"
    fi
    generator="${c_cmake_generator:-$GENERATOR_DEFAULT}"

    local args=()
    local arg
    for arg in ${c_cmake_additional_arg[@]+"${c_cmake_additional_arg[@]}"}; do
        args+=("$(expand "$arg" "$stubs")")
    done

    if output="$(configure "$project" "$build" "$generator" "$stubs" \
                     ${args[@]+"${args[@]}"} 2>&1)"; then
        status=0
    else
        status=$?
    fi

    assert_case "$project/$name" "$build" "$generator" "$stubs" "$output" "$status"
    [ -z "$stubs" ] || rm -rf "$stubs"
}

configure() {
    local project="$1" build="$2" generator="$3" stubs="$4"; shift 4
    if [ -n "$stubs" ]; then
        # CC/CXX would take precedence and skip the selection under test.
        env -u CC -u CXX PATH="$stubs:$PATH" \
            cmake -S "$HERE/$project" -B "$build" -G "$generator" \
                  --toolchain "$TOOLCHAIN" "$@" ${EXTRA_ARGS[@]+"${EXTRA_ARGS[@]}"}
    else
        cmake -S "$HERE/$project" -B "$build" -G "$generator" \
              --toolchain "$TOOLCHAIN" "$@" ${EXTRA_ARGS[@]+"${EXTRA_ARGS[@]}"}
    fi
}

expand() {
    local text="$1" stubs="$2"
    text="${text//@HERE@/$HERE}"
    text="${text//@STUBS@/$stubs}"
    text="${text//@CLANGXX@/$(detected_clang ++)}"
    text="${text//@CLANG@/$(detected_clang)}"
    text="${text//@MAJOR@/$(detected_clang_major)}"
    printf '%s' "$text"
}

assert_case() {
    local id="$1" build="$2" generator="$3" stubs="$4" output="$5" status="$6"

    if [ -n "$c_expect_message_content" ]; then
        assert_message "$id" "$(expand "$c_expect_message_content" "$stubs")" \
                       "$c_expect_message_level" "$output" "$status" || return 0
    fi
    if [ -n "$c_expect_selected_clang" ]; then
        assert_selected "$id" "$build" "$(expand "$c_expect_selected_clang" "$stubs")" \
                        "$output" "$status" || return 0
    fi
    if [ "${#c_expect_module_function[@]}" -gt 0 ] \
       || [ "${#c_forbid_module_function[@]}" -gt 0 ]; then
        assert_modules "$id" "$build" "$output" "$status" || return 0
    fi
    report ok "$id"
}

# ---------------------------------------------------------------------------
# assertions
# ---------------------------------------------------------------------------

# Prints the one diagnostic block of <kind> that contains <text>, or nothing.
# Per block, not merged: otherwise a block missing its `Fix:` line could be
# covered by a different block that has one, and the expected text could match in
# an unrelated section entirely. index() rather than a regex, because the text
# contains parentheses and dots.
diagnostic_block_with() {  # <Error|Warning> <text>
    awk -v kind="$1" -v needle="$2" '
        /^CMake (Error|Warning)/ {
            if (inside && index(block, needle)) { printf "%s", block; found = 1; exit }
            inside = ($0 ~ "^CMake " kind)
            block = inside ? $0 "\n" : ""
            next
        }
        inside { block = block $0 "\n" }
        END { if (!found && inside && index(block, needle)) printf "%s", block }
    '
}

assert_message() {
    local id="$1" text="$2" level="$3" output="$4" status="$5" block kind
    case "$level" in
        error|warning)
            kind=Error; [ "$level" = warning ] && kind=Warning
            block="$(printf '%s\n' "$output" | diagnostic_block_with "$kind" "$text")"
            if [ -z "$block" ]; then
                report fail "$id" "'$text' was not a CMake $level" "$output"; return 1
            fi
            if [ "$level" = error ] && [ "$status" -eq 0 ]; then
                report fail "$id" "configure succeeded, expected an error" "$output"; return 1
            fi
            if [ "$level" = warning ] && [ "$status" -ne 0 ]; then
                report fail "$id" "configure failed, expected only a warning" "$output"; return 1
            fi
            assert_our_diagnostic_shape "$id" "$block" || return 1 ;;
        status)
            # A status line is not part of a diagnostic block, so anchor on it.
            if ! printf '%s\n' "$output" | grep -qF -- "-- $text"; then
                report fail "$id" "'$text' was not a status line" "$output"; return 1
            fi
            if [ "$status" -ne 0 ]; then
                report fail "$id" "configure failed, expected success" "$output"; return 1
            fi ;;
    esac
}

# Only our own diagnostics must carry What/Why/Fix; they are recognisable by the
# first line the toolchain writes. Third-party messages are none of our business.
assert_our_diagnostic_shape() {
    local id="$1" blocks="$2" label
    printf '%s' "$blocks" | grep -qF 'PhasarToolchain.cmake' || return 0
    for label in "What:" "Why:" "Fix:"; do
        if ! printf '%s' "$blocks" | grep -qF -- "$label"; then
            report fail "$id" "our diagnostic has no '$label' line" "$blocks"; return 1
        fi
    done
}

# The status matters as much as the choice: PHASAR_IR_CLANG is written to the
# cache before try_compile runs, so reading the cache alone would report success
# even when configuring died right afterwards.
assert_selected() {
    local id="$1" build="$2" expected="$3" output="$4" status="$5" chosen
    if [ "$status" -ne 0 ]; then
        report fail "$id" "configuring failed" "$output"; return 1
    fi
    chosen="$(sed -n 's#^PHASAR_IR_CLANG:FILEPATH=.*/##p' "$build/CMakeCache.txt" 2>/dev/null)"
    if [ "$chosen" != "$expected" ]; then
        report fail "$id" "chose '${chosen:-<none>}', expected '$expected'" "$output"
        return 1
    fi
}

assert_modules() {
    local id="$1" build="$2" output="$3" status="$4" config configs=("") log
    if [ "$status" -ne 0 ]; then
        report fail "$id" "configuring failed" "$output"; return 1
    fi
    [ "${#c_cmake_configuration[@]}" -eq 0 ] || configs=("${c_cmake_configuration[@]}")

    for config in "${configs[@]}"; do
        local build_args=() module="$build/phasar-ir/whole-program.bc"
        if [ -n "$config" ]; then
            build_args=(--config "$config")
            module="$build/phasar-ir/$config/whole-program.bc"
        fi
        if ! log="$(cmake --build "$build" ${build_args[@]+"${build_args[@]}"} 2>&1)"; then
            report fail "$id" "build${config:+ of $config} failed" "$log"; return 1
        fi
        assert_module_functions "$id" "$module" "${config:-default}" || return 1
    done
}

assert_module_functions() {
    local id="$1" module="$2" config="$3" symbols sym
    if [ ! -s "$module" ]; then
        report fail "$id" "no module at $module"; return 1
    fi
    # shellcheck disable=SC2086 -- $DISASSEMBLER may carry arguments
    symbols="$($DISASSEMBLER "$module" -o - | grep '^define' || true)"
    if [ -z "$symbols" ]; then
        report fail "$id" "module defines no function at all ($config)"; return 1
    fi
    # Anchored on the signature parenthesis; otherwise @main would also be
    # satisfied by @main_helper and a forbidden symbol could slip through.
    for sym in ${c_expect_module_function[@]+"${c_expect_module_function[@]}"}; do
        if ! printf '%s' "$symbols" | grep -qF -- "$sym("; then
            report fail "$id" "missing $sym ($config)"; return 1
        fi
    done
    for sym in ${c_forbid_module_function[@]+"${c_forbid_module_function[@]}"}; do
        if printf '%s' "$symbols" | grep -qF -- "$sym("; then
            report fail "$id" "unexpected $sym ($config)"; return 1
        fi
    done
}

# ---------------------------------------------------------------------------
# stubs — only for compilers that cannot be installed
# ---------------------------------------------------------------------------

write_stubs() {
    local dir="$1"; shift
    local real_cc real_cxx entry name version tool
    real_cc="$(real_driver "")"
    real_cxx="$(real_driver "++")"
    [ -n "$real_cc" ] && [ -n "$real_cxx" ] \
        || die "no real clang/clang++ to back the stubs"

    for entry in "$@"; do
        name="${entry%%=*}"
        version="${entry##*=}"
        case "$name" in
            *++*) write_clang_stub "$dir/$name" "$version" "$real_cxx" ;;
            *)    write_clang_stub "$dir/$name" "$version" "$real_cc"
                  # wllvm derives the C++ driver from the C one, so provide both.
                  write_clang_stub "$dir/${name/clang/clang++}" "$version" "$real_cxx" ;;
        esac
        # extract-bc needs these for the stubbed version, or configuring aborts
        # on a missing linker instead of exercising the selection.
        for tool in llvm-link llvm-ar; do
            printf '#!/bin/sh\nexec "%s" "$@"\n' "$(find_versioned_tool "$tool")" \
                > "$dir/$tool-$version"
            chmod +x "$dir/$tool-$version"
        done
    done
}

write_clang_stub() {
    cat > "$1" <<EOF
#!/bin/sh
case "\$*" in *--version*) echo "clang version $2.0.0"; exit 0;; esac
exec "$3" "\$@"
EOF
    chmod +x "$1"
}

# Hard-coding a version here would only work on hosts that happen to have it,
# and "${cc}++" is wrong for versioned names: clang-20 pairs with clang++-20.
real_driver() {
    local major
    major="$(detected_clang_major)"
    command -v "clang${1-}-$major" 2>/dev/null && return 0
    command -v "clang${1-}" 2>/dev/null && return 0
    return 0
}

find_versioned_tool() {
    local tool="$1" v
    for v in 22 21 20 19 18 17 16; do
        command -v "$tool-$v" >/dev/null 2>&1 && { command -v "$tool-$v"; return; }
        [ -x "/usr/lib/llvm-$v/bin/$tool" ] && { echo "/usr/lib/llvm-$v/bin/$tool"; return; }
    done
    command -v "$tool"
}

# ---------------------------------------------------------------------------
# reporting
# ---------------------------------------------------------------------------

report() {  # <ok|fail> <id> [detail] [output]
    if [ "$1" = ok ]; then
        PASSED=$((PASSED + 1))
        printf 'PASS  %-38s %s\n' "$2" "$c_description"
    else
        FAILED=$((FAILED + 1))
        printf 'FAIL  %-38s %s\n' "$2" "$c_description"
        [ -n "${3:-}" ] && printf '        %s\n' "$3"
        [ -n "${4:-}" ] && printf '%s\n' "$4" | tail -12 | sed 's/^/        | /'
    fi
    return 0
}

main "$@"
