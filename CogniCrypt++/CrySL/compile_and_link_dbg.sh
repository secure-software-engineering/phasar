set -e
clang++ -I /usr/local/include/antlr4-runtime/ -I include/Parser -I include/FormatConverter -I ../../include lib/*/*.cpp lib/*/*/*.cpp main.cpp -L /usr/local/lib/ -lantlr4-runtime -std=c++17 -o cryslparser -lpthread -g -lstdc++fs
echo "Build successful"
