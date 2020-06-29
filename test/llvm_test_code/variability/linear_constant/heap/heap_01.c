
typedef unsigned long long size_t;

void *malloc(size_t size);
void free(void *ptr);

int main() { 
    int *p = malloc(sizeof(int));
    *p = 42;
    int x = *p;
    free(p);
    return 0;
}
