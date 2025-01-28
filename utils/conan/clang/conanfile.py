import json
import re
import textwrap
from pathlib import Path

from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.build import check_min_cppstd
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout
from conan.tools.env import VirtualBuildEnv
from conan.tools.files import (
    apply_conandata_patches,
    copy,
    export_conandata_patches,
    get,
    collect_libs,
    replace_in_file,
    rm,
    rmdir,
    save,
    load,
    rename
)
from conan.tools.microsoft import check_min_vs, is_msvc, is_msvc_static_runtime
from conan.tools.scm import Version
from os.path import join


required_conan_version = ">=2.0"

class ClangConan(ConanFile):
    name = "clang"
    description = "The Clang project provides a language front-end and tooling infrastructure for languages in the C language family"
    license = "Apache-2 with LLVM-exception"
    url = "https://github.com/conan-io/conan-center-index"
    homepage = "https://clang.llvm.org/"
    topics = ("clang", "llvm", "compiler")
    package_type = "library"
    settings = "os", "arch", "compiler", "build_type"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "with_xml2": [True, False],
    }
    default_options = {
        "shared": False,
        "fPIC": True,
        "with_xml2": True # LLVM_ENABLE_LIBXML2
    }

    @property
    def _min_cppstd(self):
        return 17

    @property
    def _compilers_minimum_version(self):
        return {
            "apple-clang": "10",
            "clang": "7",
            "gcc": "7",
            "msvc": "191",
            "Visual Studio": "15",
        }

    def export_sources(self):
        export_conandata_patches(self)

    def layout(self):
        cmake_layout(self, src_folder="src")

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def configure(self):
        if self.options.shared:
            self.options.rm_safe("fPIC")

    def build_requirements(self):
        self.tool_requires("ninja/[>=1.10.2 <2]")
        # needed to build c-index-test but not actually required by any components
        if self.options.with_xml2:
            # 2.5.3 min required by llvm 14
            self.test_requires("libxml2/[>=2.5.3 <3]")

    def requirements(self):
        self.requires(f"llvm-core/{self.version}", transitive_headers=True, transitive_libs=True)

    def validate(self):
        if self.settings.compiler.cppstd:
            check_min_cppstd(self, self._min_cppstd)
        minimum_version = self._compilers_minimum_version.get(str(self.settings.compiler), False)
        if minimum_version and Version(self.settings.compiler.version) < minimum_version:
            raise ConanInvalidConfiguration(
                f"{self.ref} requires C++{self._min_cppstd}, which your compiler does not support."
            )

    def source(self):
        get(self, **self.conan_data["sources"][self.version])

        clang_folder=f"clang-{self.version}.src"
        rename(self, "cmake/Modules", "cmake/modules")
        copy(self, pattern="*", src=clang_folder, dst=".")
        rmdir(self, clang_folder)

    @property
    def _get_llvm_path(self):
        return Path(self.dependencies["llvm-core"].package_folder).resolve().as_posix()

    def generate(self):
        cmake_tc = CMakeToolchain(self, generator="Ninja")
        cmake_variables = {
            "CMAKE_MODULE_PATH": join(self._get_llvm_path, "lib", "cmake", "llvm"),
            # "LLVM_TOOLS_INSTALL_DIR": "bin",
            # "LLVM_INCLUDE_TESTS": False,
            "CLANG_INCLUDE_DOCS": False,
        }
        cmake_tc.cache_variables.update(cmake_variables)
        cmake_tc.generate()

        cmake_deps = CMakeDeps(self)
        cmake_deps.generate()

    def build(self):
        apply_conandata_patches(self)
        cmake = CMake(self)
        cmake.configure(cli_args=['--graphviz=graph/clang.dot'])
        cmake.build()

    def _clang_build_info(self):
        def _sanitized_components(dependencies):
            is_link_only = re.compile(r"""\\\$<LINK_ONLY:(.+)>""")

            # CMake Target package::component -> Conan package::component
            replacements = {
                "LibXml2::LibXml2": "libxml2::libxml2",
            }
            for dep in dependencies.split(";"):
                match = is_link_only.search(dep)
                if match:
                    yield match.group(1)
                else:
                    replacement = replacements.get(dep)
                    if replacement:
                        yield replacement
                    elif dep.startswith("-l"):
                        yield dep[2:]
                    elif dep.startswith("LLVM"):
                        yield f"llvm-core::{dep}"
                    else:
                        yield dep

        def _parse_deps(deps):
            data = {
                "requires": [],
                "system_libs": [],
            }
            windows_system_libs = [
                "ole32",
                "delayimp",
                "shell32",
                "advapi32",
                "-delayload:shell32.dll",
                "uuid",
                "psapi",
                "-delayload:ole32.dll"
            ]
            for component in _sanitized_components(deps):
                if component in windows_system_libs:
                    continue
                if component in ["rt", "m", "dl", "pthread"]:
                    data["system_libs"].append(component)
                else:
                    data["requires"].append(component)
            return data

        targets = load(self, self.package_folder / self._cmake_module_path / "ClangTargets.cmake")

        match_libraries = re.compile(r'''^add_library\((\S+).*\)$''', re.MULTILINE)
        libraries = set(match_libraries.findall(targets))

        match_dependencies = re.compile(
            r'''^set_target_properties\((\S+).*\n?\s*INTERFACE_LINK_LIBRARIES\s+"(\S+)"''', re.MULTILINE)

        components = {}
        for component, dependencies in match_dependencies.findall(targets):
            if component in libraries:
                if component in components:
                    components[component].update(_parse_deps(dependencies))
                else:
                    components[component] = _parse_deps(dependencies)
        return components

    @property
    def _cmake_module_path(self):
        return Path("lib") / "cmake" / "clang"

    @property
    def _build_module_file(self):
        return self._cmake_module_path / f"conan-official-{self.name}-variables.cmake"

    @property
    def _components_path(self):
        return Path(self.package_folder) / self._cmake_module_path / "components.json"

    def _create_cmake_build_module(self, module_file):
        package_folder = Path(self.package_folder)
        content = textwrap.dedent(f"""\
            set(CLANG_INSTALL_PREFIX "{str(package_folder)}")
            set(CLANG_CMAKE_DIR "{str(package_folder / self._cmake_module_path)}")
            if (NOT TARGET clang-tablegen-targets)
              add_custom_target(clang-tablegen-targets)
            endif()
            list(APPEND CMAKE_MODULE_PATH "${{LLVM_CMAKE_DIR}}")
            # should have been be included by AddClang but isn't
            include(AddLLVM)
           """)
        save(self, module_file, content)

    def _write_build_info(self):
        components = self._clang_build_info()
        with open(self._components_path, "w", encoding="utf-8") as fp:
            json.dump(components, fp)

    def _read_build_info(self):
        with open(self._components_path, "r", encoding="utf-8") as fp:
            return json.load(fp)

    def package(self):
        copy(self, "LICENSE.TXT", self.source_folder, join(self.package_folder, "licenses"))
        cmake = CMake(self)
        cmake.install()

        self._write_build_info()
        package_folder = Path(self.package_folder)
        self._create_cmake_build_module(package_folder / self._build_module_file)

        cmake_folder = package_folder / self._cmake_module_path
        rm(self, "ClangConfig*", cmake_folder)
        rm(self, "ClangTargets*", cmake_folder)
        rmdir(self, package_folder / "share")

    def package_info(self):
        def _add_no_rtti_flag(component):
            if is_msvc(self):
                component.cxxflags.append("/GS-")
            else:
                component.cxxflags.append("-fno-rtti")

        # def _lib_name_from_component(name):
        #     replacements = {
        #         "libclang": "clang"
        #     }
        #     return replacements.get(name, name)

        self.cpp_info.set_property("cmake_file_name", "Clang")
        self.cpp_info.set_property("cmake_build_modules",
                                   [self._build_module_file,
                                    self._cmake_module_path / "AddClang.cmake"]
                                   )

        self.cpp_info.builddirs.append(self._cmake_module_path)

        llvm = self.dependencies["llvm-core"]
        if not llvm.options.rtti:
            _add_no_rtti_flag(self.cpp_info)

        if not self.options.shared:
            components = self._read_build_info()
            for component_name, data in components.items():
                self.cpp_info.components[component_name].set_property("cmake_target_name", component_name)
                self.cpp_info.components[component_name].libs = [component_name]
                if data["requires"] is not None:
                    self.cpp_info.components[component_name].requires += data["requires"]
                if data["system_libs"] is not None:
                    self.cpp_info.components[component_name].system_libs += data["system_libs"]
                if not llvm.options.rtti:
                    _add_no_rtti_flag(self.cpp_info.components[component_name])
        else:
            self.cpp_info.set_property("cmake_target_name", "Clang")
            self.cpp_info.libs = collect_libs(self)
