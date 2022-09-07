#!/bin/bash
set -euo pipefail

(
    cd "$(dirname "$0")/.."

    createStructure() {
        mkdir -p "phasar/$1/include"
        mkdir -p "phasar/$1/src"
        mkdir -p "phasar/$1/resource"
        mkdir -p "phasar/$1/test/include"
        mkdir -p "phasar/$1/test/src"
        mkdir -p "phasar/$1/test/resource"
    }

    moveInclude() {
        mapfile -t headers < <(cd "include/" && find . -wholename "*phasar/$1/*.h*" -or -wholename "*phasar/$1/*.def*")
        for header in "${headers[@]}"; do
            mkdir -p "$(dirname "phasar/$2/include/$header")"
            git mv "include/$header" "phasar/$2/include/$header"
        done
    }

    moveSrc() {
        mapfile -t sources < <(cd "lib/$1/" && find . -iname "*.cpp")
        for src in "${sources[@]}"; do
            mkdir -p "$(dirname "phasar/$2/src/$src")"
            git mv "lib/$1/$src" "phasar/$2/src/$src"
        done

    }

    moveUnittests() {
        if [ -d "unittests/$1/" ]; then
            mapfile -t sources < <(cd "unittests/$1/" && find . -iname "*.cpp")
            for src in "${sources[@]}"; do
                mkdir -p "$(dirname "phasar/$2/test/src/$src")"
                git mv "unittests/$1/$src" "phasar/$2/test/src/$src"
            done

            mapfile -t headers < <(cd "unittests/$1/" && find . -iname "*.h*")
            for header in "${headers[@]}"; do
                mkdir -p "$(dirname "phasar/$2/test/include/$header")"
                git mv "unittests/$1/$header" "phasar/$2/test/include/$header"
            done
        fi
    }

    processTarget() {
        createStructure "$2"
        moveInclude "$1" "$2"
        moveSrc "$1" "$2"
        moveUnittests "$1" "$2"
    }

    processTarget Config config
    processTarget Controller controller
    processTarget DB db
    processTarget Experimental experimental
    processTarget PhasarClang clang
    processTarget PhasarPass pass
    processTarget Utils utils
    processTarget PhasarLLVM llvm

    mapfile -t folders < <(find ./include/ ./lib/ ./unittests/ ./phasar/ -type d)
    for folder in "${folders[@]}"; do
        if [ -d "$folder" ]; then
            mapfile -t files < <(find "$folder" -type f)
            if [ "${#files[@]}" -eq "0" ]; then
                git rm -rf "$folder" &> /dev/null || true
            fi
        fi
    done

    todo="check_these_files_manually"
    git rm "$todo" &> /dev/null || true

    # check if folder $1 is migrated, $2-$n are passed to find
    check() {
        folder="$1"
        shift
        mapfile -t missed < <(find ./"$folder" -type f "$@")
        count="${#missed[@]}"
        if [ "$count" -eq "0" ]; then
            echo "$folder: all files auto migrated, removing folder"
            git rm -rf "$folder" &> /dev/null || true
            rm -rf "$folder" &> /dev/null || true
        else
            echo "$folder: couldn't auto migrate $count files"
            printf "%s\n" "${missed[@]}" >> "$todo"

            # remove ignored
            mapfile -t delete < <(find ./"$folder" -type f -not \( "$@" \))
            git rm "${delete[@]}" &> /dev/null || true
        fi
    }

    check "include" -not -name "CMakeLists.txt"

    # plugins are removed in public phasar
    git rm -rf lib/PhasarLLVM/Plugins examples/plugins &> /dev/null || true
    check "lib" -not -name "CMakeLists.txt" -and -not -iname "*-config.cmake"

    # stuff moved manually
    mkdir -p  phasar/test-utils/include/
    git mv unittests/TestUtils/TestConfig.h phasar/test-utils/include/TestConfig.h
    check "unittests" -not -name "CMakeLists.txt" -and -not -name ".clang-tidy"

    # replaced CMakeLists
    git rm -rf cmake/{phasar_macros,limit-ninja-jobs,dependencies}.cmake &> /dev/null || true
    check "cmake"

    mkdir -p phasar/llvm/{include,resource}/config/ phasar/config/resource/config/
    git mv config/DOTGraphConfig.json phasar/llvm/resource/config/
    git mv config/TaintConfigSchema.json phasar/llvm/include/config/
    git mv config/* phasar/config/resource/config/
    check "config"

    # doxyfile updated/moved to in-cmake-configuration
    check "docs" -not -name "Doxyfile.in" -and -not -name "README.dox"
    check "out" -not -name ".gitkeep"

    mkdir -p phasar/{boomerang,example-tool,clang,llvm}/src/ 
    git mv tools/boomerang/boomerang.cpp phasar/boomerang/src/boomerang.cpp
    git mv tools/example-tool/myphasartool.cpp phasar/example-tool/src/myphasartool.cpp
    git mv tools/phasar-clang/phasar-clang.cpp phasar/clang/src/phasar-clang.cpp
    git mv tools/phasar-llvm/phasar-llvm.cpp phasar/llvm/src/phasar-llvm.cpp
    check "tools" -not -name "CMakeLists.txt"

    mkdir -p phasar/llvm/include/
    git mv phasar-llvm_more_help.txt phasar/llvm/include/phasar-llvm_more_help.txt
    
    # file unused
    git rm phasar-clang_more_help.txt &> /dev/null || true
    git rm config.h.in &> /dev/null || true # never included and PHASAR_SRC_DIR PHASAR_BUILD_DIR not used
) 
