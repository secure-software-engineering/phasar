#include <stdio.h>
/* IDE linear constant analysis tracks integer constants through linear ops. */
int main() {
    int a = 6;
    int b = a + 1;   /* 7  */
    int c = a * 7;   /* 42 */
    int d = b - 3;   /* 4  */
    printf("%d %d %d\n", b, c, d);
    return 0;
}
