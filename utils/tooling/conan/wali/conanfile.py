from conans import ConanFile, CMake, tools
from collections import defaultdict
import os, re, shutil, glob, json

import shutil, errno # remove before commit

class WALiOpenNWAConan(ConanFile):
    name = "wali-opennwa"
    version = "2020.07.31"
    commit = "2bb4aca02c5a5d444fd038e8aa3eecd7d1ccbb99"
    license = "not set"
    author = "not set"
    url = "https://github.com/pdschubert/WALi-OpenNWA"
    description = "WALi weighted automaton library"
    topics = ("WALi", "automaton")
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "shared": [True, False]
    }
    default_options = {
        "shared": False
    }
    generators = "cmake_find_package", "cmake_paths"
    exports_sources = "*"
    requires = [
        "llvm-core/12.0.0@intellisectest+intellisectest-application/stable"
    ]

    @property
    def _source_subfolder(self):
        return "source"

    @property
    def _build_subfolder(self):
        return "build"

    def source(self):
        tools.get(**self.conan_data["sources"][self.version])
        tools.rename('{}-{}'.format("WALi-OpenNWA", self.commit), self._source_subfolder)

    def _patch_sources(self):
        for patch in self.conan_data.get("patches", {}).get(self.version, []):
            tools.patch(**patch)

    def build(self):
        # workaround to test packaging faster
        self._patch_sources()
        cmake = CMake(self)
        cmake.definitions['BUILD_SHARED_LIBS'] = self.options.shared
        
        
        saved_build_dir = "/home/ubuntu/hdd_2/tmp/conan_build/"
        build_dir = os.path.join(self.build_folder, self._build_subfolder)
        print(f"build folder: {build_dir} cache folder: {saved_build_dir}")
        
        if not os.path.isdir(saved_build_dir):
            os.mkdir(saved_build_dir)
            try:
                shutil.copytree(saved_build_dir, build_dir, dirs_exist_ok=True)
            except OSError as exc:
                print(exc)
                raise 
        #cmake.configure(source_folder=self._source_subfolder, build_folder=self._build_subfolder)
        #cmake.build()
        shutil.copytree(build_dir, saved_build_dir, dirs_exist_ok=True)
        

    def package(self):
        self.copy("LICENSE")
        self.copy("*.h", dst="include", src=self._source_subfolder + "/Source/wali/include")
        self.copy("*.hpp", dst="include", src=self._source_subfolder + "/Source/wali/include")
        self.copy("*.dll", dst="bin", keep_path=False)
        self.copy("*.so", dst="lib", keep_path=False)
        self.copy("*.dylib", dst="lib", keep_path=False)
        self.copy("*.a", dst="lib", keep_path=False)

    def package_info(self):
        self.cpp_info.libs = tools.collect_libs(self)
