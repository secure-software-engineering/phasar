# Building PhASAR


It is recommended to compile PhASAR yourself in order to get the full C++ experience and to have full control over the build mode.
However, you may also want to try out one of the pre-built versions of PhASAR or the Docker container.

As a shortcut for the very first PhASAR build on your system, you can use our [bootstrap](./bootstrap.sh) script.
Please note that you must have python installed for the script to work properly.

```bash
./bootstrap.sh
```

Note: If you want to do changes within PhASAR, it is recommended to build it in Debug mode:

```bash
./bootstrap.sh -DCMAKE_BUILD_TYPE=Debug
```

The bootstrap script may ask for superuser permissions (to install the dependencies); however it is not recommended to start the whole script with `sudo`.

For subsequent builds, see [Compiling PhASAR](#compiling-phasar-if-not-already-done-using-the-installation-scripts).

### Compiling PhASAR (if not already done using the bootstrap script)

Set the system's variables for the C and C++ compiler to clang:

```bash
export CC=/usr/local/bin/clang
export CXX=/usr/local/bin/clang++
```

You may need to adjust the paths according to your system. When you cloned PhASAR from Github you need to initialize PhASAR's submodules before building it:

```bash
git submodule update --init
```

If you downloaded PhASAR as a compressed release (e.g. .zip or .tar.gz) you can use the `init-submodules-release.sh` script that manually clones the required submodules:

```bash
utils/init-submodules-release.sh
```

Navigate into the PhASAR directory. The following commands will do the job and compile the PhASAR framework:

```bash
mkdir build
cd build/
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ..
ninja -j $(nproc) # or use a different number of cores to compile it
sudo ninja install # only if you wish to install PhASAR system wide
```

When you have used the `bootstrap.sh` script to install PhASAR, the above steps are already done.
Use them as a reference if you wish to modify PhASAR and recompile it.

After compilation using cmake the following two binaries can be found in the build/tools directory:

+ `phasar-cli` - the PhASAR command-line tool (previously called `phasar-llvm`) that provides access to analyses that are already implemented within PhASAR. Use this if you don't want to build an own tool on top of PhASAR.
+ `myphasartool` - an example tool that shows how tools can be build on top of PhASAR

Please be careful and check if errors occur during the compilation.

When using CMake to compile PhASAR the following optional parameters can be used:

| Parameter : Type|  Effect |
|-----------|--------|
| **BUILD_SHARED_LIBS** : BOOL | Build shared libraries -- Not recommended anymore. You may want to use PHASAR_BUILD_DYNLIB instead (default is OFF) |
| **PHASAR_BUILD_DYNLIB** : BOOL | Build one fat shared library (default is OFF) |
| **CMAKE_BUILD_TYPE** : STRING | Build PhASAR in 'Debug', 'RelWithDebInfo' or 'Release' mode (default is 'Debug') |
| **CMAKE_INSTALL_PREFIX** : PATH | Path where PhASAR will be installed if "ninja install” is invoked or the “install” target is built (default is /usr/local/phasar) |
| **PHASAR_CUSTOM_CONFIG_INSTALL_DIR** : PATH | If set, customizes the directory, where configuration files for PhASAR are installed (default is /usr/local/.phasar-config)|
| **PHASAR_ENABLE_DYNAMIC_LOG** : BOOL|Makes it possible to switch the logger on and off at runtime (default is ON)|
| **PHASAR_BUILD_DOC** : BOOL | Build PhASAR documentation (default is OFF) |
| **PHASAR_BUILD_UNITTESTS** : BOOL | Build PhASAR unit tests (default is ON) |
| **PHASAR_BUILD_IR** : BOOL | Build PhASAR IR (required for running the unit tests) (default is ON) |
| **PHASAR_BUILD_OPENSSL_TS_UNITTESTS** : BOOL | Build PhASAR unit tests that require OpenSSL (default is OFF) |
| **PHASAR_ENABLE_PAMM** : STRING | Enable the performance measurement mechanism ('Off', 'Core' or 'Full', default is Off) |
| **PHASAR_ENABLE_PIC** : BOOL | Build Position-Independed Code (default is ON) |
| **PHASAR_ENABLE_WARNINGS** : BOOL | Enable compiler warnings (default is ON) |
| **CMAKE_CXX_STANDARD** : INT|Adapt the used C++ standard (minimum required is 20)|
| **PHASAR_LLVM_VERSION** : INT|The LLVM major-version to use. Can be 16 or 17 (default is 16)|

You can use these parameters either directly or modify the installer-script `bootstrap.sh`

#### A Remark on Compile Time

C++'s long compile times are always a pain. As shown in the above, when using cmake the compilation can easily be run in parallel, resulting in shorter compilation times. Make use of it!

### Running a Test Solver

To test if everything works as expected please run the following command:

`$ phasar-cli -m test/llvm_test_code/basic/module_cpp.ll -D ifds-solvertest`

You can find the `phasar-cli` tool in the build-tree under `tools/phasar-cli`.

If you obtain output other than a segmentation fault or an exception terminating the program abnormally everything works as expected.

### Building PhASAR on a MacOS System

Due to unfortunate updates to MacOS and the handling of C++, especially on the newer M1 processors, we can't support native development on Mac.
The easiest solution to develop PhASAR on a Mac right now is to use [dockers development environments](https://docs.docker.com/desktop/dev-environments/). Clone this repository as described in their documentation. Afterwards, you have to login once manually, as a root user by running `docker exec -it -u root <container name> /bin/bash` to complete the rest of the build process as described in this readme (install submodules, run bootstrap.sh, ...).
Now you can just attach your docker container to VS Code or any other IDE, which supports remote development.

## Installation

PhASAR can be installed using the installer scripts as explained in the following.
However, you do not need to install PhASAR in order to use it.

### Installing PhASAR on an Ubuntu System

In the following, we would like to give an complete example of how to install
PhASAR using an Ubuntu or Unix-like system.

Therefore, we provide an installation script. To install PhASAR, just navigate to the top-level
directory of PhASAR and use the following command:

```bash
./bootstrap.sh --install
```

The bootstrap script may ask for superuser permissions.

Done!

If You have already built phasar, you can just invoke
```bash
sudo ninja install
```
