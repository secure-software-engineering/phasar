
#include <stdlib.h>

int main() { 
    int *p = malloc(sizeof(int));
    *p = 42;
    int x = *p;
    free(p);
    return 0;
}
