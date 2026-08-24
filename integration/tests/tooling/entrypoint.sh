#!/bin/sh
# entrypoint.sh — run the test suite as the owner of the mounted workspace.
#
# The suite configures and compiles foreign CMake code, so it must not run as
# root. Which user it should be is not configured but discovered: with a bind
# mount, the workspace directory already belongs to the host user, so adopting
# that owner makes every produced artifact readable and removable on the host
# without a chown afterwards.

set -eu

WORKSPACE=/work
DRIVER="$WORKSPACE/tests/run-all.sh"

main() {
    if [ "$(id -u)" -ne 0 ]; then
        # Already unprivileged, e.g. because the caller passed --user.
        exec "$DRIVER" "$@"
    fi

    uid="$(stat -c '%u' "$WORKSPACE")"
    gid="$(stat -c '%g' "$WORKSPACE")"
    if [ "$uid" -eq 0 ]; then
        # Dropping to nobody instead would "work" and then fail 26 times on an
        # unwritable workspace, which reads like a toolchain problem.
        printf 'entrypoint\n' >&2
        printf 'What: %s is owned by root\n' "$WORKSPACE" >&2
        printf 'Why: the suite must not run as root, and an unprivileged user cannot write build directories there\n' >&2
        printf 'Fix: mount a directory owned by your user, or pass --user <uid>:<gid>\n' >&2
        exit 4
    fi

    # The target uid usually has no passwd entry, so HOME must be set explicitly.
    export HOME=/tmp
    exec setpriv --reuid "$uid" --regid "$gid" --clear-groups "$DRIVER" "$@"
}

main "$@"
