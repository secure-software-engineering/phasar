set -e
clang++ -c -isystem /usr/local/include/antlr4-runtime/ -I /usr/local/include -I include/Parser -I include/FormatConverter -I ../../include lib/*/*.cpp lib/*/*/*.cpp -std=c++17 
echo "Compiled successful"