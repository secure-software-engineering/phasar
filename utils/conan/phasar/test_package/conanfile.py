import os

from conans import ConanFile, CMake, tools
from conans.errors import ConanInvalidConfiguration

class PhasarTestConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "cmake"

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def imports(self):
        self.copy("*.dll", dst="bin", src="bin")
        self.copy("*.dylib*", dst="bin", src="lib")
        self.copy('*.so*', dst='bin', src='lib')

    def test(self):
        if not tools.cross_building(self.settings):
            command = [
                os.path.join('bin', 'myphasartool'),
                os.path.join(os.path.dirname(__file__), 'example.ll')
            ]
            self.run(command, run_environment=True)
