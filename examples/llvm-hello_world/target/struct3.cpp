struct S {
	int x;
	int y;
	S(int i, int j) : x(i), y(j) {}
};

int main() {
	S *s = new S(10, 20);
	int sum = s->x + s->y;
	delete s;
	return sum;
}
