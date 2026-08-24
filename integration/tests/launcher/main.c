/* Compiles only when the project's compiler launcher was honoured. */
#ifndef PHASAR_LAUNCHER_MARKER
#error "compiler launcher set by the project was dropped"
#endif
int main(void) { return 0; }
