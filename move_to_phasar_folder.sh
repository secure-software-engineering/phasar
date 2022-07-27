#!/bin/bash
set -e
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
        git mv "include/phasar/$1/$header" "phasar/$2/include/$header"
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
            rm -rf "$folder"
        fi
    fi
done
