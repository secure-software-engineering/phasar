from io import StringIO

from conan import ConanFile
from conan.tools.build import can_run
from conan.tools.cmake import cmake_layout, CMake
import os

class TestPackageConan(ConanFile):
    settings = "os", "arch", "compiler", "build_type"
    test_type = "explicit"
    generators = "CMakeDeps", "CMakeToolchain", "VirtualRunEnv"

    def requirements(self):
        self.requires(self.tested_reference_str, run=True)

    def layout(self):
        cmake_layout(self)

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def _test_libtooling(self):
        bin_path = os.path.join(self.cpp.build.bindir, "test_package")
        self.run(f"{bin_path} --help" , env="conanrun")

    def _test_clang(self):
        clang = self.dependencies["clang"]
        clang_path = os.path.join(clang.package_folder, "bin", "clang")
        output = StringIO()
        self.run("which clang", env="conanrun", stdout=output)
        assert clang_path in output.getvalue().splitlines()

    def test(self):
        if can_run(self):
            self._test_libtooling()
            self._test_clang()
