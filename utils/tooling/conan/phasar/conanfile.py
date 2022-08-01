import glob, os
from distutils.dir_util import copy_tree
from conans import ConanFile, CMake, tools
from conans.model.version import Version

# How to run this recipe?
# build local phasar:
# 1. copy patches, test_package, conandata.yml, conanfile.py to your phasar repository
# 2. modify phasar
# 3. create a conan package with: conan create --build=missing . develop@user/channel
#    - version develop is important!
#    - user and channel can be whatever you like, e.g. your name as user and channel = nightly
# 4. If the command is successful it will run myphasartool with example.ll, you can now consume it with: phasar/develop@user/channel
# optional. if some changes will not be part of online phasar, create git patches, update patches folder and conandata.yml
#
# build remote phasar:
# use the provided scripts in this folder!
class PhasarConan(ConanFile):
    name = "phasar"
    license = "MIT license"
    author = "Philipp Schubert"
    url = "https://github.com/secure-software-engineering/phasar"
    description = "A LLVM-based static analysis framework. "
    topics = ("LLVM", "PhASAR", "SAST")
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "shared": [True, False],
        "plugins_shared": [True, False],
        # plugins arent needed for library usage and cant be build/linked statically
        "plugins_removed": [True],
    }
    default_options = {
        "shared": False,
        "plugins_shared": False,
        "plugins_removed": True
    }
    generators = "cmake_find_package", "cmake_paths"
    exports_sources = "*"
    requires = [
        # llvm-core from conan-center only allows shared = False, because with shared the ci job needs to much resources
        "llvm-core/12.0.0@intellisectest+intellisectest-application/stable",
        "nlohmann_json/3.9.1",
        # phasar needs boost_thread which can only be linked shared: https://www.boost.org/doc/libs/1_76_0/libs/config/doc/html/index.html
        "boost/1.76.0@intellisectest+intellisectest-application/stable",
        "gtest/1.10.0@intellisectest+intellisectest-application/stable",
        "sqlite3/3.36.0",
        "wali-opennwa/2020.07.31@intellisectest+intellisectest-application/stable",
        "json-schema-validator/2.1.0"
    ]

    def package_id(self):
        self.info.settings.compiler.version = str(self.info.settings.compiler.version) + "_" + "boost:shared=" + str(self.options["boost"].shared)

    @property
    def _source_subfolder(self):
        if self.version == 'develop':
            return "."
        else:
            return "source"

    @property
    def _build_subfolder(self):
        return "build"

    def source(self):
        if self.version != 'develop':
            tools.get(**self.conan_data["sources"][self.version])
            tools.rename(glob.glob("phasar-*")[0], self._source_subfolder)

    def _patch_sources(self):
        for patch in self.conan_data.get("patches", {}).get(self.version, []):
            tools.patch(**patch)
        
        if not self.options.plugins_shared:
            tools.replace_in_file(self._source_subfolder + "/lib/PhasarLLVM/Plugins/CMakeLists.txt", "plugin_name} SHARED", "plugin_name} STATIC")
        
        if self.options.plugins_removed:
            tools.replace_in_file(self._source_subfolder + "/lib/PhasarLLVM/Plugins/CMakeLists.txt", "file(GLOB_RECURSE PLUGINS_SO ", "#file(GLOB_RECURSE PLUGINS_SO ")

    def build(self):
        self._patch_sources()
        cmake = CMake(self)
        
        if self.options["boost"].shared:
            cmake.definitions['BOOST_ALL_DYN_LINK'] = True # used from patched CMakeLists
        else:
            cmake.definitions['Boost_USE_STATIC_LIBS'] = True # provided by cmake FindBoost

        cmake.definitions['BUILD_SHARED_LIBS'] = self.options.shared
        cmake.definitions['PHASAR_BUILD_UNITTESTS'] = False
        cmake.definitions['PHASAR_BUILD_IR'] = False
        cmake.definitions['PHASAR_USE_SYSTEM_GTEST'] = True
        cmake.definitions['PHASAR_USE_SYSTEM_JSON'] = True
        cmake.definitions['PHASAR_USE_SYSTEM_WALI'] = True
        cmake.definitions['PHASAR_USE_SYSTEM_JSON_SCHEMA_VALIDATOR'] = True
        cmake.configure(source_folder=self._source_subfolder, build_folder=self._build_subfolder)
        cmake.build()

    def package(self):
        tools.rename(self._source_subfolder + "/LICENSE.txt", self._source_subfolder + "/LICENSE")
        self.copy("LICENSE")
        # todo copy other licenses too
        self.copy("*.def", dst="include", src=self._source_subfolder + "/include")
        self.copy("*.h", dst="include", src=self._source_subfolder + "/include")
        self.copy("*.hpp", dst="include", src=self._source_subfolder + "/include")
        self.copy("*.dll", dst="bin", src=self._build_subfolder, keep_path=False)
        self.copy("*.so", dst="lib", src=self._build_subfolder, keep_path=False)
        self.copy("*.dylib", dst="lib", src=self._build_subfolder, keep_path=False)
        self.copy("*.a", dst="lib", src=self._build_subfolder, keep_path=False)

    def package_info(self):
        self.cpp_info.libs = tools.collect_libs(self)
