#!/bin/bash

# author: Philipp D. Schubert
#
# This scripts determines the compiler information and
# standard header includes paths and places them in two
# configuration files that are placed in the config/ directory.

ConfigDir=config/
CompilerInfoFile=compiler_info.txt
StdHeaderPathFile=standard_header_paths.conf

echo -e "int main() { return 0; }" | clang++ -x c++ -v - -o /dev/null &> ${CompilerInfoFile}
if [[ "$OSTYPE" == "linux-gnu" ]]; then
	echo "system: Linux"
	cat ${CompilerInfoFile} | tr -d '\n' | sed -e 's/.*#include <\.\.\.> search starts here: \(.*\)End of search list\..*/\1/' | tr ' ' '\n' > ${StdHeaderPathFile}
elif [[ "$OSTYPE" == "darwin"* ]]; then
	echo "system: Mac OSX"
	sed -i -e 's/(framework directory)//g' ${CompilerInfoFile}
	cat ${CompilerInfoFile}| tr -d '\n' | sed -e 's/.*#include <\.\.\.> search starts here: \(.*\)End of search list\..*/\1/' |tr ' ' '\n' > ${StdHeaderPathFile}
	sed -i -e '/^\s*$/d' ${StdHeaderPathFile}
	rm ${CompilerInfoFile}-e
	rm ${StdHeaderPathFile}-e

else
	echo "OS not supported yet, abort!"
fi
mv ${CompilerInfoFile} ${ConfigDir}
mv ${StdHeaderPathFile} ${ConfigDir}

