from distutils.dir_util import copy_tree
from conans import ConanFile, CMake, tools
from conans.errors import ConanInvalidConfiguration
import glob

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
        "shared": [True, False]
    }
    default_options = {
        "shared": False
    }
    generators = "cmake"
    
    requires = [
        "llvm/[>=14.0.0 <15.0.0]@phasar/develop",
        "boost/[>=1.72.0 <=1.81.0]",
        "gtest/[>=1.10.0 <2.0.0]",
        "sqlite3/[>=3.36.0 <4.0.0]",
        "json-schema-validator/[>=2.1.0 <3.0.0]",
        "nlohmann_json/[>=3.10.5 <3.11.0]",
        "zlib/[>=1.2.0 <2.0.0]" # fix boost / clash zlib
    ]

    def set_version(self):
        git = tools.Git()
        calver = git.run("show -s --date=format:'%Y.%m.%d' --format='%cd'")
        short_hash = git.run("show -s --format='%h'")
        self.version = f"{calver}+{short_hash}"
        # XXX extract version from root CMakeLists.txt

    def requirements(self):
        self.options['llvm'].enable_debug = True
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
        
        # workaround as long as the pr into upstream phasar is open
        setattr(self.options['llvm'], "with_runtime_compiler-rt", True)
        
    # hint: only executed during create if recipe and source is in same repo, def set_version() would also be called during install
    def export_sources(self):
        self.copy("../../../*", excludes=["build", "cmake-build", ".github", ".git", ".vscode"])

    def build_requirements(self):
        self.tool_requires("cmake/[>=3.21.0 <4.0.0]")
        self.tool_requires("ninja/[>=1.9.0 <2.0.0]")

    def _configure_cmake(self):
        cmake = CMake(self, generator='Ninja')
        cmake.definitions['BUILD_SHARED_LIBS'] = self.options.shared
        cmake.definitions['PHASAR_BUILD_UNITTESTS'] = False
        cmake.configure()
        return cmake

    def build(self):
        cmake = self._configure_cmake()
        cmake.build()

    def package(self):
        cmake = self._configure_cmake()
        cmake.install()

        self.copy("LICENSE.txt", src=self.source_folder, dst="licenses")

    def package_info(self):
        self.cpp_info.libs = tools.collect_libs(self)
