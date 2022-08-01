# Conan overview

## using gitlab package registry as conan remote

### installing

`pip install conan` currently version 1.44

### setup remote for Intellisec

1. Get your Gitlab Project ID e.g. [`16796`](https://gitlab.cc-asp.fraunhofer.de/iem_paderborn/projects/intellisectest/intellisectest-application) or [`7787`](https://gitlab.cc-asp.fraunhofer.de/lbriese/gittoowncloud)
2. [Create a personal access token with `read_api` or optional `api`](https://gitlab.cc-asp.fraunhofer.de/-/profile/personal_access_tokens)
3. add the remote for project id: `conan remote add itst https://gitlab.cc-asp.fraunhofer.de/api/v4/projects/16796/packages/conan`
4. Use your token to login: `conan user -r "itst" -p "TOKEN" "USER"`
5. (optional) Because of some gitlab issues, create a script called conan-relog with:
```bash
#!/bin/bash

conan user --clean
conan user -r "itst" -p "TOKEN" "randomUser"
```

### setup local for Intellisec

1. `conan config init`
2. `conan profile update settings.compiler=clang default`
3. `conan profile update settings.compiler.version=12 default`
4. `conan profile update settings.compiler.libcxx=libstdc++11 default`
5. `conan config set general.request_timeout=300` our gitlab needs sometimes around 60s to verify e.g. that a 500MB file is uploaded correctly but 60 is default

## using conan

### searching

- find new packages: [Conan Center](https://conan.io/center/)
- watch recipes of official packages: [Conan Center Index](https://github.com/conan-io/conan-center-index/)
- `conan search` show all local packages
- `conan search "pha*"` show all local packages which starts with "pha"
- `conan search "phasar" -r itst` show every phasar version on remote

### I need some changes to the sources

1. Clone the original project in the specific version you want to modify
2. Do your changes
3. Create a git patch from your changes
   - `git diff > my.patch` for every unstaged changes
   - `git diff --cached > my.patch` for every stages changes
4. Open the recipe folder e.g. `phasar` which contains:
   - the recipe to create the package `conanfile.py`
   - the `patches` folder, add your patch here
   - `conandata.yml` add your patch to patches structure for specific version
     - Do you want to create a new package revision - old one will be still there? Just add your patch
     - Do you want to create a new package version? Just create a new element below `sources` and `patches`, copy from the existing elements and add your changes only to the new version
     - For phasar this is enough because the recipe iterates over this structure to applies the patches e.g.:
      ```
      for patch in self.conan_data.get("patches", {}).get(self.version, []):
         tools.patch(**patch)
      ```
5. Create the packages
   - Prefer the scripts to create / upload
   - Else `conan create --build=missing . "VersionYouWantToBuild@user/channel"` e.g. user/channel could be `intellisectest+intellisectest-application/stable`
6. Upload it `conan upload "$NAME/$VERSION@$TARGET" -r "$REMOTE" --all --force`
   - replace $NAME, $VERSION, $TARGET
   - replace Target remote
   - `--all` upload recipe and binary, without only upload recipe
   - `--force` overwrite remote recipe without confirmation

### other

- Limit Conan CPUS with environment variable `CONAN_CPU_COUNT=1` e.g. `CONAN_CPU_COUNT=1 bash llvm-core/create.sh`
- Execute a ../conanfile.txt from build directory: `conan install ..` or `conan install --build=missing ..`
- Which options are available for given package? `conan inspect boost/1.76.0`
- Create a blank conan package right here: `conan new package_name/version -t`
- Execute recipe and tag it with user/channel: `conan create . user/channel` [package naming for Gitlab](https://gitlab.cc-asp.fraunhofer.de/help/user/packages/conan_repository/index.md#package-recipe-naming-convention-for-instance-remotes)
- Upload a package to specific remote `itst` including recipe and binary: `conan upload "phasar/$VERSION@intellisectest+intellisectest-application/stable:PACKAGE_ID" -r itst --all --check -c`

## integrating conan

### cmake

Result:
- `cmake .. && cmake --build .` will work
- You dont need to invoke conan directly!

```
if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
   message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
   file(DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/v0.16.1/conan.cmake"
                  "${CMAKE_BINARY_DIR}/conan.cmake"
                  EXPECTED_HASH SHA256=396e16d0f5eabdc6a14afddbcfff62a54a7ee75c6da23f32f7a31bc85db23484
                  TLS_VERIFY ON)
endif()
include(${CMAKE_BINARY_DIR}/conan.cmake)
conan_cmake_run(
    BASIC_SETUP
    CONANFILE "${CMAKE_SOURCE_DIR}/conanfile.txt" 
    BUILD missing)
```

### conanfile.txt

[official reference](https://docs.conan.io/en/latest/reference/conanfile_txt.html)

1. Not every projects needs a conanfile.txt you can also do everything from cmake if you include `conan.cmake`.
2. Section `[requires]` takes a recipe reference `package_name/version` from conan-center or `package_name/version@user/channel`
   - for intellisec gitlab user: `intellisectest+intellisectest-application` which references the the application package registry
3. Section `[options]` allows to set options on every `package_name` e.g. `boost:shared=True`
   - inspect available options and its default values with `conan inspect package_name`

### Alternative cmake integration 

ToDo for different generators

## Example: Add Conan dependency and provide others the binary

1. Ensure you added the remote `itst` with `conan remote list`
2. find what you want to add: [searching section](###searching)
3. add it to the section '[requires]' e.g. `boost/1.76.0`
4. check the options you need: `conan inspect boost/1.76.0`
5. add options to `conanfile.txt` like `boost:shared=True`
6. compile the application as Release and as Debug, this will be propagated to the dependencies
  - `cmake -DCMAKE_BUILD_TYPE=Release .. && cmake --build .`
  - `cmake -DCMAKE_BUILD_TYPE=Debug .. && cmake --build .`
5. We now have every dependency with provided settings, with provided options and with provided build type in our conan repository.
6. copy recipe and created binaries to our new target user/channel `conan copy --all --force boost/1.76.0 intellisectest+intellisectest-application/stable`
7. upload them `conan upload boost/1.76.0@intellisectest+intellisectest-application/stable -r itst --all --check -c`
8. update conanfile section '[requires]' e.g. change `boost/1.76.0` to `boost/1.76.0@intellisectest+intellisectest-application/stable`
   - if its a indirect dependency e.g. `phasar` needing `llvm-core/12.0.0` just add `llvm-core/12.0.0@intellisectest+intellisectest-application/stable` to `[requires]` of your application
9.  clear build directory and build it once to check if everything is working
10. commit, push, tell others

## glossar
- recipe reference: `package_name/version@user/channel`
  - for conan-center `package_name/version` or `package_name/version@_/_`
  - `package_name/version@project/stable` `package_name/version@project/develop`
  - for intellisec gitlab user: `intellisectest+intellisectest-application` which references the the application package registry
- package id: hash of configuration
- `conanbuildinfo.cmake` / `conanbuildinfo.txt` generated by `conan install ..` or by `conan.cmake` will list every target / include / lib / ...
