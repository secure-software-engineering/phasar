int main() {
    int x = 0, y = 0;
    int *p = &x;
    int *q = p;    /* q aliases p (both point to x) */
    int *r = &y;   /* r points to y (must-not-alias p/q) */
    *q = 5;
    return *p + *r;
}
