from conan import ConanFile
from conan.tools.build import check_min_cppstd
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout


class Lab7ClientConan(ConanFile):
    name = "lab7-client"
    version = "0.1.0"
    package_type = "static-library"
    settings = "os", "arch", "compiler", "build_type"
    exports_sources = "CMakeLists.txt", "cmake/*", "include/*", "src/*"

    def requirements(self):
        self.requires("cppzmq/4.10.0")
        self.requires("nlohmann_json/3.11.3")

    def validate(self):
        if self.settings.compiler.get_safe("cppstd"):
            check_min_cppstd(self, "20")

    def layout(self):
        cmake_layout(self)

    def generate(self):
        CMakeDeps(self).generate()
        toolchain = CMakeToolchain(self)
        toolchain.variables["BUILD_BROKER"] = False
        toolchain.variables["FETCH_DEPENDENCIES"] = False
        toolchain.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        CMake(self).install()

    def package_info(self):
        self.cpp_info.set_property("cmake_file_name", "Lab7Client")

        client = self.cpp_info.components["client"]
        client.set_property("cmake_target_name", "Lab7::Client")
        client.libs = ["lab7_client"]
        client.requires = ["cppzmq::cppzmq"]

        protocol = self.cpp_info.components["protocol"]
        protocol.set_property("cmake_target_name", "Lab7::Protocol")
        protocol.libs = ["lab7_protocol"]
        protocol.requires = ["nlohmann_json::nlohmann_json"]
