set -e

NUM_THREADS=$(nproc)


sudo pip install Pygments
sudo pip install pyyaml
sudo apt-get install libboost-all-dev

#verify installation
BOOST_VERSION=$(echo -e '#include <boost/version.hpp>\nBOOST_LIB_VERSION' | gcc -s -x c++ -E - 2>/dev/null| grep "^[^#;]" | tr -d '\"') 
if [ -z $BOOST_VERSION ] ;then
    echo "Failed installing boost $DESIRED_BOOST_VERSION"
    exit 1
else
    echo "Successfully installed boost v${BOOST_VERSION//_/.}"
fi




# installing LLVM
./utils/install-llvm-8.0.0.sh $NUM_THREADS ./utils/
# installing wllvm
sudo pip3 install wllvm

echo "dependencies successfully installed"