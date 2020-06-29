
typedef struct _X {
    int i;
    int j;
    #ifdef ADDITIONAL_INT
    int k;
    #endif
} X;

int main() {
    X x;
    x.i = 10;
    x.j = 20;
    #ifdef ADDITIONAL_INT
    x.k = 30;
    #endif
    return 0;
}
