#include<iostream>

struct S{

    S () {
        std::cout << "Struct Test\n";
    }
};

S s;

int main(){

    return 0;
}