from distutils.dir_util import copy_tree
from conans import ConanFile, CMake, tools
from conans.model.version import Version
import glob, os

enforce_local_build = True

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
    homepage = "https://github.com/secure-software-engineering/phasar"
    description = "A LLVM-based static analysis framework. "
    topics = ("LLVM", "PhASAR", "SAST")
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "shared": [True, False],

        # build only options
        "is_recipe_and_source_in_same_repo": [True, False],
    }
    default_options = {
        "shared": False,

        # build only options
        "is_recipe_and_source_in_same_repo": enforce_local_build
    }
    generators = "cmake"
    
    requires = [
        "llvm/14.0.6@phasar/develop",
        "boost/[>=1.72.0 <1.77]",
        "gtest/[>=1.10.0 <2.0.0]",
        "sqlite3/[>=3.36.0 <4.0.0]",
        "json-schema-validator/[>=2.1.0 <3.0.0]",
        "nlohmann_json/[>=3.10.5 <4.0.0]",
        "zlib/[>=1.2.0 <2.0.0]" # fix boost / clash zlib
    ]
        
    def export_sources(self):
        if enforce_local_build: # self.options arent allowed in this method!
            self.copy("../../../../*", excludes=["build", "cmake-build", ".github", ".cache", ".git"])
        else:
            self.copy("*")

    def build_requirements(self):
        self.tool_requires("cmake/[>=3.21.0 <4.0.0]")
        self.tool_requires("ninja/[>=1.9.0 <2.0.0]")

    def configure(self):
        self.options['llvm'].with_project_clang = True
        self.options['llvm'].with_project_openmp = True

        # actually not needed because unittests are off
        # it currently is just simplier compared to passing -o llvm:keep_binaries_regex="^(clang|clang\+\+|opt)$" all the time. 
        # Other solution would be to modify the llvm recipe package_id:
        # if keep_binaries_regex="^$" append compatible_pkg "ANY" but you can only put there concrete values, no wildcards, so to specific for conan-center
        self.options['llvm'].keep_binaries_regex = "^(clang|clang\+\+|opt)$"

        if self.options.shared:
            self.options['llvm'].shared = True
            self.options['sqlite3'].shared = True

        if self.options.get_safe('is_recipe_and_source_in_same_repo', default=enforce_local_build):
            self.version = "develop"
        # TODO extract version from root CMakeLists.txt

    def source(self):
        if not self.options.get_safe('is_recipe_and_source_in_same_repo', default=enforce_local_build):
            tools.get(**self.conan_data["sources"][self.version])
            tools.rename(glob.glob("phasar-*")[0], self._source_subfolder)
        # else: export_sources prepared the sources already

        try:
            for patch in self.conan_data.get("patches", {}).get(self.version, []):
                tools.patch(**patch)
        except TypeError:
            self.output.info(f"no patches found for version \"{self.version}\"")

    def _configure_cmake(self):
        cmake = CMake(self, generator='Ninja')
        cmake.definitions['BUILD_SHARED_LIBS'] = self.options.shared
        cmake.definitions['PHASAR_BUILD_UNITTESTS'] = False
        cmake.configure()
        return cmake

    def build(self):
        cmake = self._configure_cmake()
        cmake.build()

    def package_id(self):
        del self.options.is_recipe_and_source_in_same_repo

    def package(self):
        cmake = self._configure_cmake()
        cmake.install()

        self.copy("LICENSE.txt", src=self.source_folder, dst="licenses")

    def package_info(self):
        self.cpp_info.libs = tools.collect_libs(self)
