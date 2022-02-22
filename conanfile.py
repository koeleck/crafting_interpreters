from conans import ConanFile, CMake, tools
import os

class CraftingInterpretersConan(ConanFile):
    name = "crafting_interpreters"
    version = "0.1"
    settings = "os", "compiler", "build_type", "arch"
    generators = "cmake_paths", "cmake_find_package"

    def requirements(self):
        self.requires("catch2/2.13.8")
        self.requires("fmt/8.1.1")

    def source(self):
        pass

    def build(self):
        cmake = self._get_cmake()
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = self._get_cmake()
        cmake.install()

    def package_info(self):
        pass

    def _get_cmake(self):
        cmake = CMake(self)
        conan_paths_cmake = os.path.join(self.install_folder, "conan_paths.cmake")
        cmake.definitions["CMAKE_PROJECT_crafting_interpreters_INCLUDE"] = conan_paths_cmake
        return cmake
