
from conans import ConanFile, CMake, tools
from conans.tools import os_info, SystemPackageTool
from pathlib import Path
import os
import shutil


class uwebsocketConan(ConanFile):
    name = "usockets"
    version = "0.0.5"
    license = "Apache 2.0"
    author = "kaiyin keezhong@qq.com"
    url = "https://github.com/kindlychung/uSockets"
    description = "µSockets - miniscule networking & eventing"
    topics = ("c", "network", "sockets")
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False]}
    default_options = "shared=False"
    generators = "cmake"
    exports_sources = "src/*"

    def source(self):
        git = tools.Git(folder="usockets")
        git.clone(self.url, "cmake_build")

    def build(self):
        cmake = CMake(self)
        cmake.configure(source_folder="src")
        cmake.build()

    def package(self):
        self.copy("interfaces/*", dst="include", src="src", keep_path=True)
        self.copy("internal/*", dst="include", src="src", keep_path=True)
        self.copy("*.h", dst="include", src="src", keep_path=False)
        self.copy("*.lib", dst="lib", keep_path=False)
        self.copy("*.dll", dst="bin", keep_path=False)
        self.copy("*.dylib*", dst="lib", keep_path=False)
        self.copy("*.so", dst="lib", keep_path=False)
        self.copy("*.a", dst="lib", keep_path=False)

    def imports(self):
        self.copy("*", dst="include", src="include")
        self.copy("*", dst="bin", src="lib")

    def package_info(self):
        self.cpp_info.libs = ["usockets"]
