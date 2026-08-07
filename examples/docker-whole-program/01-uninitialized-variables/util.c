#include "util.h"
int compute(int seed) {
    int result;            /* BUG: not initialized on the seed<=0 path */
    if (seed > 0) {
        result = seed * 2;
    }
    return result;         /* uninitialized use when seed <= 0 */
}
