set -e
clang++ -I /usr/local/include/antlr4-runtime/ -I include/Parser -I include/FormatConverter -I ../../include lib/*/*.cpp lib/*/*/*.cpp main.cpp -std=c++17 -o cryslparser -lpthread -lstdc++fs
echo "Build successful"
