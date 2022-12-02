#!/bin/bash
set -uo pipefail

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
        if [ -d include ]; then
            mapfile -t headers < <(cd "include/" && find . -wholename "*phasar/$1/*.h*" -or -wholename "*phasar/$1/*.def*")
            for header in "${headers[@]}"; do
                mkdir -p "$(dirname "phasar/$2/include/$header")"
                git mv "include/$header" "phasar/$2/include/$header"
            done
        fi
    }

    moveSrc() {
        if [ -d "lib/$1/" ]; then
            mapfile -t sources < <(cd "lib/$1/" && find . -iname "*.cpp")
            for src in "${sources[@]}"; do
                mkdir -p "$(dirname "phasar/$2/src/$src")"
                git mv "lib/$1/$src" "phasar/$2/src/$src"
            done
        fi
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
        echo "processing expected target $1 and migrate to $2"
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
    echo "simpliest migration done"
    printf '\n\n\n'

    mapfile -t folders < <(find ./include/ ./lib/ ./unittests/ ./phasar/ -type d 2>/dev/null)
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
        if [ -d ./"$folder" ]; then
            mapfile -t missed < <(find ./"$folder" -type f "$@")
            count="${#missed[@]}"
            if [ "$count" -eq "0" ]; then
                echo "$folder: all files auto migrated, removing folder"
                git rm -rf "$folder" &> /dev/null || true
                rm -rf "$folder" &> /dev/null || true
            else
                echo "$folder: couldn't auto migrate $count files"
                printf "%s\n" "${missed[@]}" | tee --append "$todo"

                # remove ignored
                mapfile -t delete < <(find ./"$folder" -type f -not \( "$@" \))
                git rm "${delete[@]}" &> /dev/null || true
            fi
        fi
    }

    check "include" -not -name "CMakeLists.txt"

    # plugins are removed in public phasar
    git rm -rf lib/PhasarLLVM/Plugins examples/plugins &> /dev/null || true
    check "lib" -not -name "CMakeLists.txt" -and -not -iname "*-config.cmake"

    # stuff moved manually
    mkdir -p  phasar/test-utils/include/
    git mv unittests/TestUtils/TestConfig.h phasar/test-utils/include/TestConfig.h 2>/dev/null
    check "unittests" -not -name "CMakeLists.txt" -and -not -name ".clang-tidy"

    # replaced CMakeLists
    git rm -rf cmake/{phasar_macros,limit-ninja-jobs,dependencies}.cmake &> /dev/null || true
    
    mkdir -p phasar/llvm/{include,resource}/config/ phasar/config/resource/config/
    git mv config/DOTGraphConfig.json phasar/llvm/resource/config/ 2>/dev/null
    git mv config/TaintConfigSchema.json phasar/llvm/include/config/ 2>/dev/null
    git mv config/* phasar/config/resource/config/ 2>/dev/null
    check "config"

    # doxyfile updated/moved to in-cmake-configuration
    check "docs" -not -name "Doxyfile.in" -and -not -name "README.dox"
    check "out" -not -name ".gitkeep"

    mkdir -p phasar/{boomerang,example-tool,clang,llvm}/src/
    git mv tools/boomerang/boomerang.cpp phasar/boomerang/src/boomerang.cpp 2>/dev/null
    git mv tools/example-tool/myphasartool.cpp phasar/example-tool/src/myphasartool.cpp 2>/dev/null
    git mv tools/phasar-clang/phasar-clang.cpp phasar/clang/src/phasar-clang.cpp 2>/dev/null
    git mv tools/phasar-llvm/phasar-llvm.cpp phasar/llvm/src/phasar-llvm.cpp 2>/dev/null
    check "tools" -not -name "CMakeLists.txt"

    mkdir -p phasar/llvm/include/
    git mv phasar-llvm_more_help.txt phasar/llvm/include/phasar-llvm_more_help.txt 2>/dev/null
    
    # file unused
    git rm phasar-clang_more_help.txt &> /dev/null || true
    git rm config.h.in &> /dev/null || true # never included and PHASAR_SRC_DIR PHASAR_BUILD_DIR not used

    # files moved because different include folders and other targets depends on it
    # e.g. some files where part of phasar-llvm but are used on direct dependencies of phasar-llvm => moved to utils
    moved() {
        src="$(realpath $1)"
        target="$(realpath $2)"
        if [ -f "$src" ]; then
            old_sum="$(md5sum "$src" | grep -Eo '^[^ ]*')"
            new_sum="$(md5sum "$target" | grep -Eo '^[^ ]*')"
            
            # if identical we can safely remove the file from upstream, because here we have a correct commit message!
            if [ -n "$old_sum" ] && [ "$old_sum" = "$new_sum" ]; then
                echo "[auto] removing file where git lost track, checksum is identical:"
                git rm -f "$src"
                git add "$target"
            else
                printf "please merge contents of \nold location: \"%s\"\nnew location:\"%s\"\nmanually" "$src" "$target"
            fi
        fi
    }
    #moved ./phasar/llvm/include/phasar/PhasarLLVM/Utils/LLVMCXXShorthands.h ./phasar/utils/include/phasar/Utils/LLVMCXXShorthands.h
    #moved ./phasar/llvm/src/Utils/LLVMCXXShorthands.cpp ./phasar/utils/src/LLVMCXXShorthands.cpp
    #moved ./phasar/llvm/include/phasar/PhasarLLVM/Utils/LLVMShorthands.h ./phasar/llvm/include/phasar/Utils/LLVMShorthands.h
    #moved ./phasar/llvm/include/phasar/PhasarLLVM/Utils/LLVMIRToSrc.h ./phasar/utils/include/phasar/Utils/LLVMIRToSrc.h
    #moved ./lib/PhasarLLVM/Utils/LLVMShorthands.cpp ./phasar/llvm/src/Utils/LLVMShorthands.cpp
    #moved ./phasar/llvm/src/Utils/LLVMIRToSrc.cpp ./phasar/utils/src/LLVMIRToSrc.cpp
    
    # stage 2, handlinug current merge requests
    status="$(git status)"

    mapfile -t both_deleted < <(echo "$status" | grep -Po '(?<=both deleted:).*' | xargs printf '%s\n')
    echo "assuming if upstream and fork deleted a file -> we can remove it from merge"
    if [ -n "${both_deleted[*]}" ]; then
        rm "${both_deleted[@]}" 2>/dev/null
        git add "${both_deleted[@]}" 2>/dev/null
    fi
    printf '\n\n\n\n'



    mapfile -t deleted_by_them < <(echo "$status" | grep -Po '(?<=deleted by them:).*' | xargs printf '%s\n')
    echo "assuming if deleted by upstream -> we can remove it"
    if [ -n "${deleted_by_them[*]}" ]; then
        rm "${deleted_by_them[@]}" 2>/dev/null
        git add "${deleted_by_them[@]}" 2>/dev/null
    fi
    printf '\n\n\n\n'



    mapfile -t deleted_by_us < <(echo "$status" | grep -Po '(?<=deleted by us:).*' | xargs printf '%s\n')
    echo "assuming if deleted by our rework -> we can remove it"
    if [ -n "${deleted_by_us[*]}" ]; then
        rm "${deleted_by_us[@]}" 2>/dev/null
        git add "${deleted_by_us[@]}" 2>/dev/null
    fi
    printf '\n\n\n\n'



    mapfile -t added_by_them < <(echo "$status" | grep -Po '(?<=added by them:).*' | grep -v llvm_test_code | xargs printf '%s\n')
    echo "assuming if added upstream -> we need to add it, if below llvm_test_code use git mv to change the filename appropriate!"
    if [ -n "${added_by_them[*]}" ]; then
        git add "${added_by_them[@]}"
    fi
    printf '\n\n\n\n'



    # cleanup empty directories
    find . -type d -empty -delete

    mapfile -t llvm_test_code < <(echo "$status" | grep -Po '(?<=added by them:).*llvm_test_code.*' | xargs printf '%s\n')
    echo "Correct filename ending for llvm_test_code files, e.g. .dbg .o1 .m2r or combinations of it"
    printf '%s\n' "${llvm_test_code[@]}"
    printf '\n\n\n\n'

    echo "found code which may need to be adapted to new llvm_test_code because looks like the old style"
    echo "e.g. call_01_cpp_dbg.ll -> call_01.dbg.ll (most of the time, if multiple needed create a symbolic link)"
    grep -rnE '_(c(pp)?|dbg|m2r)\.ll' phasar/
    printf '\n\n\n\n'
) 
