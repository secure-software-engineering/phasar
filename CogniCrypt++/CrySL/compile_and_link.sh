set -e
clang++ -isystem /usr/local/include/antlr4-runtime/ -I /usr/local/include -I include/Parser -I include/FormatConverter -I ../../include lib/*/*.cpp lib/*/*/*.cpp main.cpp -std=c++17 -o cryslparser -lpthread -lstdc++fs
echo "Build successful"
