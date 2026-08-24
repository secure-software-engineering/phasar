#!/bin/sh
# Compiler launcher that appends a define the source insists on. If the toolchain
# drops the project's launcher, the define is missing and compilation fails — that
# is what makes "the project's launcher survives" an observable property.
exec "$@" -DPHASAR_LAUNCHER_MARKER=1
