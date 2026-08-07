#include <stdlib.h>
/* 'source' returns tainted data; 'sink' must never receive tainted data. */
extern char *source(void);
extern void sink(const char *data);
int main() {
    char *tainted = source();
    sink(tainted);           /* LEAK: tainted value reaches the sink */
    return 0;
}
