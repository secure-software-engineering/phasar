from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps
from conan.tools.build import check_min_cppstd
from conan.tools.files import (
    load,
    save,
    copy,
    rm,
)
from conan.tools.scm import Git
from os.path import join, isdir, exists
import re
from pathlib import Path, PurePosixPath
import textwrap
import json
import os
from conan.errors import ConanException

required_conan_version = ">=2.0"

def components_from_dotfile(dotfile):
    def node_labels(dot):
        # here we are mapping dependencies visible to cmake to component dependencies in conan
        label_replacements = {
            "LibXml2::LibXml2": "libxml2::libxml2",
            "ZLIB::ZLIB": "zlib::zlib",
            "zstd::libzstd_static": "zstd::zstdlib",
            "-lpthread": "pthread",
            "curl": "libcurl::libcurl",
            "nlohmann_json_schema_validator": "json-schema-validator::json-schema-validator",
            "clangCodeGen": "clang::clangCodeGen",
            "clangTooling": "clang::clangTooling",
            "SQLite::SQLite3": "sqlite3::sqlite3",
        }
        for row in dot:
            # e.g. "node0" [ label = "phasar\n(phasar::phasar)", shape = octagon ];
            match_label = re.match(r'''^\s*"(node[0-9]+)"\s*\[\s*label\s*=\s*"([^\\"]+)''', row)
            if match_label:
                node = match_label.group(1)
                label = match_label.group(2)
                if label.startswith("LLVM"):
                    yield node, f"llvm-core::{label}"
                # XXX find_library adds direct filepath -> imho a flaw in current cmake files
                elif label.endswith("libsqlite3.a"):
                    yield node, "sqlite3::sqlite3"
                elif label.endswith("libclang-cpp.so"):
                    yield node, "clang::clang"
                elif label.endswith("libclangCodeGen.a"):
                    yield node, "clang::clangCodeGen"
                elif label.endswith("libclangTooling.a"):
                    yield node, "clang::clangTooling"
                else:
                    yield node, label_replacements.get(label, label)

    def node_dependencies(dot):
        ignore_deps = [
        ]
        labels = {k: v for k, v in node_labels(dot)}
        for row in dot:
            # "node0" -> "node1" [ style = dashed ] // phasar -> LLVMAnalysis
            match_dep = re.match(r'''^\s*"(node[0-9]+)"\s*->\s*"(node[0-9]+)".*''', row)
            if match_dep:
                node_label = labels[match_dep.group(1)]
                dependency = labels[match_dep.group(2)]
                if node_label.startswith("phasar") and PurePosixPath(dependency).parts[-1] not in ignore_deps:
                    yield node_label, labels[match_dep.group(2)]
        # some components don't have dependencies
        for label in labels.values():
            if label.startswith("phasar"):
                yield label, None

    system_libs = {
        "ole32",
        "delayimp",
        "shell32",
        "advapi32",
        "-delayload:shell32.dll",
        "uuid",
        "psapi",
        "-delayload:ole32.dll",
        "ntdll",
        "ws2_32",
        "rt",
        "m",
        "dl",
        "pthread",
        "stdc++fs"
    }
    components = {}
    dotfile_rows = dotfile.split("\n")
    for node, dependency in node_dependencies(dotfile_rows):
        if dependency in system_libs:
            key = "system_libs"
        elif dependency is not None and (dependency.startswith("phasar") or "::" in dependency):
            key = "requires"
        else:
            key = "unknown"
        if node not in components:
            components[node] = { "system_libs": [], "requires": [], "unknown": [] }
            if dependency is not None:
                components[node][key] = [dependency]
        elif dependency is not None:
            components[node][key].append(dependency)

    return components

class PhasarRecipe(ConanFile):
    name = "phasar"
    package_type = "library"

    # Optional metadata
    license = "MIT license"
    url = "https://github.com/secure-software-engineering/phasar"
    description = "A LLVM-based static analysis framework. "
    topics = ("LLVM", "PhASAR", "SAST")

    # Binary configuration
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "with_z3": [True, False],
        "shared": [True, False],
        "fPIC": [True, False],
        "tests": [True, False],
        "llvm_version": ["ANY"],
    }
    default_options = {
        "with_z3": True,
        "shared": False,
        "fPIC": True,
        "tests": False,
        "llvm_version": "15.0.7"
    }

    def _parse_gitignore(self, folder, additional_exclusions = [], invert=False):
        exclusions = []
        inclusions = []
        if invert:
            for exc in additional_exclusions:
                if exc.startswith("!"):
                    inclusions = exc[1:]
                else:
                    inclusions = f"!{exc}"
        else:
            exclusions = additional_exclusions

        with open(f'{folder}/.gitignore', 'r') as file:
            for line in file:
                line = line.strip()
                if line.startswith("#") or not line:
                    continue
                if invert:
                    if line.startswith("!"):
                        inclusions.append(line[1:])
                    else:
                        inclusions.append("!" + line)
                else:
                    exclusions.append(line)

        if invert:
            return inclusions
        else:
            return exclusions

    def export_sources(self):
        exclusions = self._parse_gitignore(".", [
            "test_package",
            "utils",
            "img",
            "githooks",
            "external"
        ])

        for tlf in os.listdir("."):
            if isdir(tlf):
                copy(self, f"{tlf}/*", src=".", dst=self.export_sources_folder, excludes=exclusions)
            else:
                copy(self, tlf, src=".", dst=self.export_sources_folder, excludes=exclusions)

    @property
    def _graphviz_file(self):
        return PurePosixPath(self.build_folder) / "graph" / "phasar.dot"

    @property
    def _info_file(self):
        # this is called very early where folders aren't set but this is fine
        if self.export_folder is None:
            return None
        return PurePosixPath(self.export_folder) / "info.json"

    def _read_info(self):
        if self._info_file is not None and exists(self._info_file):
            with open(self._info_file, encoding="utf-8") as fp:
                return json.load(fp)
        else:
            return {
                "version": None,
            }

    def _write_info(self, info):
        if self._info_file is not None:
            with open(self._info_file, "w", encoding="utf-8") as fp:
                json.dump(info, fp, indent=2)

    def set_version(self):
        if self.version is not None:
            return
        info = self._read_info()
        version = info["version"]
        if version is None:
            git = Git(self, self.recipe_folder)
            # XXX consider git.coordinates_to_conandata()
            if git.is_dirty():
                raise ConanException("Repository is dirty. I can't calculate a correct version and this is a potential leak because all files visible to git will be exported. Please stash or commit, to skip this for local testing use \"--version dev\".")
            self.output.info("No version information set, retrieving from git.")
            calver = git.run("show -s --date=format:'%Y.%m.%d' --format='%cd'")
            short_hash = git.run("show -s --format='%h'")
            version = f"{calver}+{short_hash}"
        if info["version"] != version:
            info["version"] = version
            self._write_info(info)
        self.version = version

    def layout(self):
        cmake_layout(self)

    def config_options(self):
        if self.settings.os == "Windows":
            self.options.rm_safe("fPIC")

    def requirements(self):
        self.requires("boost/[>1.72.0 <=1.86.0]")
        self.requires("sqlite3/[>=3 <4]")
        self.requires(f"clang/{self.options.llvm_version}@secure-software-engineering", transitive_libs=True, transitive_headers=True)
        self.requires("nlohmann_json/3.11.3", transitive_headers=True)
        self.requires("json-schema-validator/2.3.0", transitive_libs=True, transitive_headers=True)

        llvm_options={
            "rtti": True,
        }
        if self.options.with_z3:
            self.requires("z3/[>=4.7.1 <5]")
            llvm_options["with_z3"] = True
        self.requires(f"llvm-core/{self.options.llvm_version}@secure-software-engineering", transitive_libs=True, transitive_headers=True, options=llvm_options)

    def build_requirements(self):
        self.tool_requires("cmake/[>=3.25.0 <4.0.0]") # find_program validator
        self.tool_requires("ninja/[>=1.9.0 <2.0.0]")
        if self.options.tests:
            self.test_requires("openssl/[>2 <4]")
            self.test_requires("gtest/1.14.0")

    def configure(self):
        if self.options.shared:
            self.options.rm_safe("fPIC")

    def validate(self):
        check_min_cppstd(self, '17')

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self, 'Ninja')
        tc.generate()

    def _cmake_configure(self):
        cmake = CMake(self)
        self._handle_graphviz()
        cmake.configure(
            variables={
                'PHASAR_ENABLE_PIC': self.options.get_safe("fPIC", False),
                'PHASAR_USE_CONAN': True,
                'BUILD_SHARED_LIBS': self.options.shared,
                'PHASAR_BUILD_UNITTESTS': self.options.tests,
                'PHASAR_BUILD_IR': self.options.tests,
                'PHASAR_BUILD_DOC': False,
                'PHASAR_USE_Z3': self.options.with_z3,
                'USE_LLVM_FAT_LIB': False,
                'BUILD_PHASAR_CLANG': True,
                'PHASAR_BUILD_TOOLS': True,
            },
            cli_args=[
                f"--graphviz={self._graphviz_file}"
            ]
        )
        return cmake

    def _handle_graphviz(self):
        exclude_patterns = [
            "LLVMTableGenGlobalISel.*",
            "CONAN_LIB.*",
            "LLVMExegesis.*",
            "LLVMCFIVerify.*"
        ]
        graphviz_options = textwrap.dedent(f"""
            set(GRAPHVIZ_EXECUTABLES OFF)
            set(GRAPHVIZ_MODULE_LIBS OFF)
            set(GRAPHVIZ_OBJECT_LIBS OFF)
            set(GRAPHVIZ_IGNORE_TARGETS "{';'.join(exclude_patterns)}")
        """)
        save(self, PurePosixPath(self.build_folder) / "CMakeGraphVizOptions.cmake", graphviz_options)

    def build(self):
        cmake = self._cmake_configure()
        cmake.build()
        if self.options.tests:
            cmake.ctest(cli_args=[
                "--exclude-regex 'IDEExtendedTaintAnalysisTest.*'", # known flaky test
                "--no-tests=error",
                "--output-on-failure",
                "--test-dir",
                self.build_folder
            ])

    @property
    def _cmake_module_path(self):
        return PurePosixPath("lib") / "cmake" / "phasar"

    @property
    def _build_info_file(self):
        return PurePosixPath(self.package_folder) / self._cmake_module_path / "conan_phasar_build_info.json"

    def _write_build_info(self):
        # maybe process original config
        cmake_config = Path(self.package_folder / self._cmake_module_path / "phasarConfig.cmake").read_text("utf-8")
        components = components_from_dotfile(load(self, self._graphviz_file))

        build_info = {
            "components": components,
        }

        with open(self._build_info_file, "w", encoding="utf-8") as fp:
            json.dump(build_info, fp, indent=2)

        return build_info

    def _read_build_info(self) -> dict:
        with open(self._build_info_file, encoding="utf-8") as fp:
            return json.load(fp)

    def package_id(self):
        del self.info.options.tests

    def package(self):
        copy(self, "LICENSE.txt", self.source_folder, join(self.package_folder, "licenses"))

        cmake = self._cmake_configure()
        cmake.install()
        rm(self, "phasarConfig*.cmake", join("lib", "cmake", "phasar"))
        rm(self, "*target*.cmake", join("lib", "cmake", "phasar"))

        self._write_build_info()

    def package_info(self):
        self.cpp_info.set_property("cmake_file_name", "phasar")

        interfaces = ["phasar_interface"]

        build_info = self._read_build_info()
        components = build_info["components"]

        for component_name, data in components.items():
            self.cpp_info.components[component_name].set_property("cmake_target_name", component_name)
            self.cpp_info.components[component_name].libs = [component_name] if component_name not in interfaces else []
            self.cpp_info.components[component_name].requires = data["requires"]
            self.cpp_info.components[component_name].system_libs = data["system_libs"]
