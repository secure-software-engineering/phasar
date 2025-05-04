#!/bin/bash
readonly LLVM_INSTALL_DIR=${1}
readonly PHASAR_INSTALL_DIR="${2}"
if [ "$#" -ne 2 ] || [ -z "${LLVM_INSTALL_DIR}" ] || [ -z "${PHASAR_INSTALL_DIR}" ];
then
	echo "usage: ./setEnvironmentVariables.sh <LLVM_INSTALL_DIR> <PHASAR_INSTALL_DIR>" >&2
	exit 1
fi

RCPATH=~/.$(basename "$SHELL")rc
EXPORTGUARD="# PhASAR export guard"

# Check whether the SHELLrc file contains the EXPORTGUARD
if grep -Fxq "$EXPORTGUARD" "$RCPATH"
then
    echo "Environment variables have already been set"
else
    echo 'Add environment variables to' "$(basename "$SHELL")"'rc'
    # LLVM exports
    echo '' >> "$RCPATH"
    echo '# LLVM' >> "$RCPATH"
    echo 'export PATH=${PATH}:'"${LLVM_INSTALL_DIR}"'/bin' >>"$RCPATH"
    echo 'export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:'"${LLVM_INSTALL_DIR}"'/lib' >>"$RCPATH"
    echo 'export LIBRARY_PATH=${LIBRARY_PATH}:'"${LLVM_INSTALL_DIR}"'/include' >>"$RCPATH"
    # PhASAR exports
    echo '# PhASAR' >>"$RCPATH"
    echo 'export PATH=${PATH}:'"${PHASAR_INSTALL_DIR}"'/bin' >>"$RCPATH"
    echo 'export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:'"${PHASAR_INSTALL_DIR}"'/lib' >>"$RCPATH"
    echo 'export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:'"${PHASAR_INSTALL_DIR}"'/lib/phasar' >>"$RCPATH"
    echo 'export LIBRARY_PATH=${LIBRARY_PATH}:'"${PHASAR_INSTALL_DIR}"'/include' >>"$RCPATH"
    echo 'export PHASAR_INCLUDE_DIR='"${PHASAR_INSTALL_DIR}"'/include' >>"$RCPATH"
    echo 'export PHASAR_LIBRARY_DIR='"${PHASAR_INSTALL_DIR}"'/lib' >>"$RCPATH"
    # Add the export guard
    echo "$EXPORTGUARD" >>"$RCPATH"
    # Open a new shell to ensure that the ENV variables are loaded
    $SHELL
fi
