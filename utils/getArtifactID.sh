#!/bin/bash

# TODO: get artifacts for all build types (Release, Debug, DebugLibdeps, DebugCov)

# Get current repository name (OWNER) (i.e. secure-software-engineering, fabianbs96, etc)
REPOS="$(git remote -v)"
SPLITREPOS=(${REPOS//// })
REPOGITLINK=${SPLITREPOS[1]}
SPLITREPOGITLINK=(${REPOGITLINK//:/ })
REPONAME=(${SPLITREPOGITLINK[1]})

# Download artifacts info list

curl https://api.github.com/repos/$REPONAME/phasar/actions/artifacts > artifacts-info.txt

# Get latest artifact (Should always be upper most one)

URLLINE="$(grep -nr artifacts-info.txt -e 'url' | head -1)"
URLLINESPLIT=(${URLLINE//\"/ })
ARTLINK=(${URLLINESPLIT[3]})
echo ${ARTLINK}

# TODO: download the artifact data into ~/.cache/ccache
# TODO: ARTLINK apparently isn't what we're looking for here, it only downloads this part of
# the artifact info list

curl $ARTLINK > test.txt
