__attribute__((constructor)) void before_main();
__attribute__((destructor)) void after_main();

void before_main() {}
void after_main() {}

int main() {}
