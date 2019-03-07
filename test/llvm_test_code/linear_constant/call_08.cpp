int decrement(int i) { 
	if (i > 0) {
		return decrement(--i);
	}
	return -1;
}

int main() {
	int i = 42;
	int j = decrement(i);
}
