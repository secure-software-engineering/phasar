
int main() {
    int *i = new int(42);
    int j = *i;
    delete i;
    return j;
}
