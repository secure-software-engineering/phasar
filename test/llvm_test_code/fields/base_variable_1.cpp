#include <iostream>

int main() {
	char str[] = "Hello, World!\n";
	char *str_ptr = str;
	std::cout << *str_ptr;
}
