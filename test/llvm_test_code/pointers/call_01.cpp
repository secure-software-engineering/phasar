
void setInteger(int *x) {
    *x = 42;
}

int main() {
	int i;
	int *p = &i;
    setInteger(p);
	*p = 13;
    return 0;
}
