
#include <cstdlib>

int main() {
	int *p = static_cast<int *>(malloc(sizeof(int)));
	*p = 13;
	free(p);
}
