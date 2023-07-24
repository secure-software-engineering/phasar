#!/bin/bash

set -euo pipefail

debugger="lldb"
preset="debug"
label=""
regex=""
while [ $# -ge 1 ]; do
    key="$1"
    shift

    case "$key" in
        -h|--help)
            echo "[help][run_debugger] run gdb on tests"
            echo "[help][run_debugger] run_debugger.sh [option]"
            echo "[help][run_debugger] "
            echo "[help][run_debugger] options:"
            echo "[help][run_debugger] -lldb                              default, use lldb"
            echo "[help][run_debugger] -gdb                               use gdb"
            echo "[help][run_debugger] -L <regex>  --label-regex <regex>"
            echo "[help][run_debugger] -p <preset  --preset      <preset> (default = debug)"
            echo "[help][run_debugger] -R <filter>  --tests-regex <filter> used as ctest filter"
            echo "[help][run_debugger] <filter>                            -R / --test-regex can be omitted"
            echo "[help][run_debugger] "
            exit 0;;
        -lldb)
            debugger="lldb"
            ;;
        -gdb)
            debugger="gdb"
            ;;
        -p|--preset)
            preset="$1"
            shift;;
        -L|--label-regex)
            label="$1"
            shift;;
        -R|--tests-regex)
            regex="$1"
            shift;;
        *)
            if [ -z "$regex" ]; then
                regex="$key"
            else
                echo "[error][run_debugger] unhandled argument \"$key\""
                exit 1
            fi;;
    esac
done

get_longest_common_subsequence() {
    start="$1"

    if [ "$#" -eq "1" ]; then
        echo "$start"
    else
        lcs=""
        for i in $(seq 0 ${#start}); do
            for j in $(seq 1 $((${#start} - $i ))); do
                current="${start: $i: $j}"

                # if match in all strings
                contained_in="$(printf "%s\n" "$@" | grep --count -Ee "^.*$current.*$")"
                if [ "$contained_in" -eq "$#" ]; then
                    if [ "${#lcs}" -lt "${#current}" ]; then
                        lcs="$current"
                    fi
                
                # no match -> there will be no longer match with this prefix
                else
                    break;
                fi
            done
        done
        echo "$lcs"
    fi
}

(
    cd "$(dirname "$0")/.."

    cmd=(ctest "--preset" "$preset" "--show-only=human" "--verbose")
    if  [ -n "$label" ]; then
        cmd+=("--label-regex" "$label")
    fi
    if [ -n "$regex" ]; then
        cmd+=("--tests-regex" "$regex")
    fi

    echo "[info][run_debugger] ${cmd[*]}"
    mapfile -t invocations < <("${cmd[@]}" | grep -Po '(?<=Test command: ).+')
    run_binary=""
    last_binary=""
    filter=()
    for invocation in "${invocations[@]}"; do
        binary="$(echo "$invocation" | awk '{ print $1 }')"
        if [ -z "$last_binary" ]; then
            last_binary="$binary"
            run_binary="$binary"
        elif [ "$last_binary" = "$binary" ]; then
            filter+=( "$(echo "$invocation" | grep -Poe '(?<=--gtest_filter=)[^ "]+')" ) 
        else
            echo "[warning][run_debugger] more as one binary, use more precise filter, will only run \"$(basename "$run_binary")\" but not \"$(basename "$binary")\""
            last_binary="$binary"
        fi
    done
    if [ -z "$run_binary" ]; then
        echo "[error][run_debugger] no test command found"
        exit 2
    fi

    echo cd "$(dirname "$run_binary")"
    cd "$(dirname "$run_binary")"
    if [ "$debugger" = "lldb" ]; then
        cmd=(lldb -- "$(basename "$run_binary")")
    else
        cmd=(gdb --args "$(basename "$run_binary")")
    fi
    
    # if no regex present execute all tests
    if [ -z "$regex" ]; then
        lcs=""

    # if regex present calculate lcs
    else
        lcs="$(get_longest_common_subsequence "${filter[@]}")"
    fi

    # no lcs = all tests in binary
    if [ -z "$lcs" ]; then
        cmd+=(--gtest_filter="*")
    else
        cmd+=(--gtest_filter="*$lcs*")
    fi
    
    echo "[info][run_debugger] ${cmd[*]}"
    "${cmd[@]}"
)
