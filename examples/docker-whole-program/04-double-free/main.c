#include <stdlib.h>
int main() {
    int *p = (int *)malloc(sizeof(int) * 4);
    free(p);
    free(p);   /* BUG: double free (p flows from one free() to another) */
    return 0;
}
