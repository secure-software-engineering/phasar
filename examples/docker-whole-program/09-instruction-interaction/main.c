#include <stdio.h>
/* IDE instruction-interaction: which instructions influence which others. */
int main() {
    int secret = 42;
    int derived = secret + 1;   /* influenced by secret */
    int unrelated = 7;          /* independent */
    printf("%d %d\n", derived, unrelated);
    return 0;
}
