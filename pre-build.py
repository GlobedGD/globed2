"""
Globed pre-build script.
This script is invoked by cmake when configuring the project, you'll find that
most of the build configuration is done here, rather than CMakeLists.txt.

This file ends up generating the mod.json and a .cmake file that gets included by the main CMakeLists.txt.
Scroll to the bottom to see the actual configuration.
"""

from __future__ import annotations
from dataclasses import dataclass, field
from pathlib import Path
from enum import Enum, auto
from subprocess import Popen, PIPE, STDOUT
import argparse
import platform
import hashlib
import time
import json
import sys

state: State

class Platform(Enum):
    Windows = auto()
    Android32 = auto()
    Android64 = auto()
    Mac = auto()
    Ios = auto()

    @staticmethod
    def parse(platform: str) -> Platform:
        platform = platform.lower()
        if platform == "windows" or platform == "win" or platform == "win64":
            return Platform.Windows
        elif platform == "android32":
            return Platform.Android32
        elif platform == "android64":
            return Platform.Android64
        elif platform == "macos":
            return Platform.Mac
        elif platform == "ios":
            return Platform.Ios
        else:
            raise ValueError(f"Unknown platform: {platform}")

    def platform_str(self, include_bit: bool = False) -> str:
        out = ""

        match self:
            case Platform.Windows:
                out = "windows"
            case Platform.Android32, Platform.Android64:
                out = "android"
            case Platform.Mac:
                out = "macos"
            case Platform.Ios:
                out = "ios"

        if include_bit:
            match self:
                case Platform.Android32:
                    out += "32"
                case Platform.Android64:
                    out += "64"

        return out

    def is_windows(self) -> bool:
        return self == Platform.Windows

    def is_ios(self) -> bool:
        return self == Platform.Ios

    def is_android(self) -> bool:
        return self in (Platform.Android32, Platform.Android64)

    def is_mac(self) -> bool:
        return self == Platform.Mac

    def is_desktop(self) -> bool:
        return self in (Platform.Windows, Platform.Mac)

    def is_mobile(self) -> bool:
        return not self.is_desktop()

    def is_apple(self) -> bool:
        return self in (Platform.Mac, Platform.Ios)

def truthy(val: str) -> bool:
    return val.lower() in ("1", "true", "yes", "on", "y")

def fatal_error(message: str):
    print(f"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!")
    print(f"!! Build halted due to error:")
    for line in message.splitlines():
        print(f"!! {line}")
    print(f"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!")
    exit(1)

@dataclass
class CPMDep:
    name: str
    repo: str
    tag: str
    options: dict[str, str]
    link_type: str

class CMakeFile:
    vars: dict[str, str]
    defs: dict[str, tuple[str, str]]
    dirs: list[tuple[Path, str]]
    libs: list[tuple[str, str]]
    messages: list[str]
    source_dirs: list[tuple[Path, bool]]
    deps: list[CPMDep]
    options: list[tuple[str | None, str, str]]

    def __init__(self, path: Path) -> None:
        self.path = path
        self.vars = {}
        self.defs = {}
        self.dirs = []
        self.libs = []
        self.messages = []
        self.source_dirs = []
        self.deps = []
        self.options = []

    def add_var(self, key: str, value: str):
        self.vars[key] = value

    def add_definition(self, key: str, value: str = "", privacy: str = "PRIVATE"):
        self.defs[key] = (value, privacy)

    def add_library(self, name: str, privacy: str = "PRIVATE"):
        self.libs.append((name, privacy))

    def add_libraries(self, *names: str, privacy: str = "PRIVATE"):
        for name in names:
            self.libs.append((name, privacy))

    def add_message(self, message: str):
        self.messages.append(message)

    def add_source_dir(self, path: Path, recursive: bool = True):
        # if directory, assume it should be globbed
        if path.is_dir():
            path = path / "*.cpp"

        if not path.parent.exists():
            raise FileNotFoundError(f"Source directory {path.parent} does not exist")

        self.source_dirs.append((path, recursive))

    def add_include_dir(self, path: Path, privacy: str = "PRIVATE"):
        self.dirs.append((path, privacy))

    def add_compile_option(self, lib: str | None, opt: str, privacy: str = "PRIVATE"):
        self.options.append((lib, opt, privacy))

    def add_cpm_dep(self, name: str, repo: str, tag: str, options: dict | None = None, link_name: str | None = None, link_type: str = "PRIVATE"):
        if options is None:
            options = {}

        self.deps.append(CPMDep(name, repo, tag, options, link_type))
        self.add_library(link_name or name, link_type)

    def convert_path(self, path: Path) -> str:
        sfx = None

        if path.is_relative_to(state.build_dir):
            sfx = path.relative_to(state.build_dir)
            return f"\"{sfx}\""
        elif path.is_relative_to(state.source_dir):
            sfx = path.relative_to(state.source_dir)
            return f"\"${{CMAKE_CURRENT_SOURCE_DIR}}/{sfx}\""
        else:
            return f'"{path.absolute()}"'


    def export_str(self) -> str:
        out = "# Generated by pre-build.py, DO NOT EDIT THIS FILE DIRECTLY\n\n"

        # Messages
        for message in self.messages:
            out += f'message(STATUS "{message}")\n'

        # Variables
        for key, value in self.vars.items():
            out += f"set({key} \"{value}\")\n"

        # Sources
        out += "\n\n# Source files\n"
        out += "set(SOURCES \"\")\n"

        source_var_names = []

        for path, recursive in self.source_dirs:
            is_glob = "*" in path.name

            if is_glob:
                converted = self.convert_path(path)
                hashed = hashlib.sha256(converted.encode()).hexdigest()[:16]

                name = f"SOURCES_{hashed}"
                assert name not in source_var_names, f"Duplicate source path: {converted}"

                source_var_names.append(name)
                glob_type = "GLOB_RECURSE" if recursive else "GLOB"
                out += f"file({glob_type} {name} CONFIGURE_DEPENDS {converted})\n"
            else:
                out += f"list(APPEND SOURCES \"{self.convert_path(path)}\")\n"

        for name in source_var_names:
            out += f"list(APPEND SOURCES ${{{name}}})\n"

        # Add library
        out += "\nadd_library(${PROJECT_NAME} SHARED ${SOURCES})\n"

        # Definitions
        for key, (value, privacy) in self.defs.items():
            val_str = f"={value}"
            out += f"target_compile_definitions(${{PROJECT_NAME}} {privacy} {key}{val_str})\n"

        # Include dirs
        out += "\n# Include directories\n"
        for path, privacy in self.dirs:
            out += f"target_include_directories(${{PROJECT_NAME}} {privacy} {self.convert_path(path)})\n"

        # CPM deps
        out += "\n# CPM Dependencies\n"
        for dep in self.deps:
            url = ""
            if dep.repo.startswith("http"):
                url = dep.repo
            else:
                url = f"https://github.com/{dep.repo}.git"

            out += f"CPMAddPackage(\n"
            out += f'    NAME {dep.name}\n'
            out += f'    GIT_REPOSITORY "{url}"\n'
            out += f'    GIT_TAG "{dep.tag}"\n'
            if len(dep.options) > 0:
                out += f'    OPTIONS '
                for (opt_key, opt_value) in dep.options.items():
                    out += f'    "{opt_key} {opt_value}"\n'

            out += f")\n"

        # Options
        out += "\n# Compile options\n"
        for lib, opt, privacy in self.options:
            lib = lib or "${PROJECT_NAME}"
            out += f"target_compile_options({lib} {privacy} {opt})\n"

        # Links
        out += "\n# Linked libraries\n"
        for name, privacy in self.libs:
            if '/' in name:
                name = self.convert_path(Path(name))

            out += f"target_link_libraries(${{PROJECT_NAME}} {privacy} {name})\n"

        return out

    def save(self):
        self.path.write_text(self.export_str())

@dataclass
class State:
    source_dir: Path
    build_dir: Path
    cmake: CMakeFile
    platform: Platform
    debug: bool
    release: bool
    voice_support: bool
    oss_build: bool
    geode_sdk_path: Path
    server_url: str
    modules: list[str]
    compiler_id: str
    compiler_frontend: str
    compiler_version: str
    require_geode: str

    extra_deps: dict[str, str] = field(default_factory=dict[str, str])

    def add_module(self, module: str):
        self.modules.append(module)

    def has_module(self, module: str) -> bool:
        return module in self.modules

    def is_clang(self) -> bool:
        return "clang" in self.compiler_id.lower()

    def is_clang_cl(self) -> bool:
        return "clang" in self.compiler_id.lower() and self.compiler_frontend == "MSVC"

    def add_extra_dep(self, key: str, version: str):
        self.extra_deps[key] = version

    def silence_warnings_for(self, lib: str):
        opt = "/w" if self.is_clang_cl() else "-w"
        self.cmake.add_compile_option(lib, opt, "PRIVATE")

    def host_desc(self) -> str:
        if sys.platform == "linux":
            data = platform.freedesktop_os_release()
            name = data.get("PRETTY_NAME", None) or data.get("NAME", "Unknown Linux")
            return f"{name} ({platform.uname().machine})"
        elif sys.platform == "darwin":
            ver, _, machine = platform.mac_ver()
            return f"macOS {ver} ({machine})"
        elif sys.platform == "win32":
            ver = platform.version()
            arch = platform.machine()
            return f"Windows {ver} ({arch})"
        else:
            return f"Unknown"

    def print_info(self):
        print(f"========== Globed build configuration ==========")
        print(f"Platform: {self.platform.name}, host: {self.host_desc()}, debug: {self.debug}, release: {self.release}, OSS build: {self.oss_build}")
        print(f"Voice: {self.voice_support}, modules: {self.modules}, server URL: '{self.server_url}'")
        print(f"Compiler: {self.compiler_id} {self.compiler_version}, frontend: '{self.compiler_frontend}'")
        print("=================================================")

    def invoke_git(self, where: Path, *args) -> tuple[int, str]:
        proc = Popen(["git", *args], cwd=where, stdout=PIPE, stderr=STDOUT)
        assert proc.stdout

        output = proc.stdout.read().decode().strip()
        code = proc.wait()

        return (code, output)

    def is_sdk_at_least(self, ver: str) -> bool:
        code, output = self.invoke_git(self.geode_sdk_path, "merge-base", "--is-ancestor", ver, "HEAD")

        if code == 0:
            return True
        else:
            print(output)
            return False

    def get_sdk_commit(self) -> str | None:
        code, output = self.invoke_git(self.geode_sdk_path, "rev-parse", "HEAD")

        if code != 0:
            # fatal_error(f"Failed to get Geode SDK commit:\n{output}")
            return None

        return output

    def verify_geode_version(self):
        if not self.is_sdk_at_least(self.require_geode):
            msg = "Geode version mismatch! Please update Geode SDK to build Globed.\n"
            if 'v' in self.require_geode or '.' in self.require_geode:
                msg += f"Required Geode version: {self.require_geode}\n"
            else:
                msg += f"Geode nightly is required (at least commit {self.require_geode}), run `geode sdk update nightly`\n"

            msg += f"Current Geode commit: {self.get_sdk_commit() or 'unknown'}\n"

            fatal_error(msg)

    def generate_mod_json(self):
        in_path = self.source_dir / "mod.json.template"
        out_path = self.source_dir / "mod.json"

        data = json.loads(in_path.read_text())

        for (key, ver) in self.extra_deps.items():
            data["dependencies"][key] = ver

        # if not self.release:
        #     data["_comment"] = "NOTE: this file is auto generated, please DO NOT edit it, instead edit mod.json.template"

        out_path.write_text(json.dumps(data, indent=4))

    def generate_cmake(self):
        self.cmake.save()

if __name__ == '__main__':
    start_time = time.time()

    parser = argparse.ArgumentParser()
    parser.add_argument('--build-dir', required=True, type=Path)
    parser.add_argument('--output-file', required=True, type=Path)
    parser.add_argument('--modules', type=str)
    parser.add_argument('--default-server-url', type=str)
    parser.add_argument('--platform', required=True, type=str)
    parser.add_argument('--debug', required=True, type=str)
    parser.add_argument('--release', required=True, type=str)
    parser.add_argument('--oss-build', required=True, type=str)
    parser.add_argument('--geode-sdk-path', required=True, type=Path)
    parser.add_argument('--voice-support', type=str)
    parser.add_argument('--server-shared-ver', type=str)
    parser.add_argument('--qunet-ver', type=str)
    parser.add_argument('--quic', type=str)
    parser.add_argument('--advanced-dns', type=str)
    parser.add_argument('--compiler-id', required=True, type=str)
    parser.add_argument('--compiler-frontend', required=True, type=str)
    parser.add_argument('--compiler-version', required=True, type=str)
    parser.add_argument('--require-geode', type=str)
    args = parser.parse_args()

    state = State(
        source_dir=Path(__file__).parent,
        build_dir=args.build_dir,
        cmake=CMakeFile(args.output_file),
        platform=Platform.parse(args.platform),
        debug=truthy(args.debug),
        release=truthy(args.release),
        voice_support=truthy(args.voice_support),
        oss_build=truthy(args.oss_build),
        geode_sdk_path=args.geode_sdk_path,
        server_url=args.default_server_url or "",
        modules=args.modules.split(",") if args.modules else [],
        compiler_id=args.compiler_id,
        compiler_frontend=args.compiler_frontend,
        compiler_version=args.compiler_version,
        require_geode=args.require_geode,
    )

    state.print_info()

    # check some preconditions, such as needing geode nightly or regular clang
    if state.is_clang_cl() or not state.is_clang():
        fatal_error("Clang-cl and MSVC are not supported, Globed can only be built with Clang")

    state.verify_geode_version()

    # Add necessary modules
    state.add_module("deathlink")
    state.add_module("two-player")
    state.add_module("ui")

    # Add base sources
    src = state.source_dir / "src"
    state.cmake.add_source_dir(src / "core")
    state.cmake.add_source_dir(src / "audio")
    state.cmake.add_source_dir(src / "util")
    state.cmake.add_source_dir(src / "ui")
    state.cmake.add_source_dir(src / "soft-link")
    state.cmake.add_source_dir(src / "platform" / state.platform.platform_str())

    # Include dirs
    state.cmake.add_include_dir(state.source_dir / "include", "PUBLIC")
    state.cmake.add_include_dir(state.source_dir / "src")
    state.cmake.add_include_dir(state.source_dir / "libs")

    # Compile definitions / variables
    if state.release:
        state.cmake.add_var("CMAKE_INTERPROCEDURAL_OPTIMIZATION", "ON")

    state.cmake.add_definition("GLOBED_BUILD")
    state.cmake.add_definition("GLOBED_DEFAULT_MAIN_SERVER_URL", f'"{state.server_url}"')
    state.cmake.add_definition("UIBUILDER_NO_ARROW", "1")

    if state.debug:
        state.cmake.add_definition("GLOBED_DEBUG", "1")
        state.cmake.add_var("QUNET_DEBUG", "ON")

    if state.voice_support:
        state.cmake.add_definition("GLOBED_VOICE_SUPPORT", "1")
        if state.platform.is_windows():
            state.cmake.add_definition("GLOBED_VOICE_CAN_TALK", "1")

    # Add geode dependencies based on modules
    if state.has_module('scripting-ui'):
        state.add_extra_dep("alphalaneous.editortab_api", ">=1.0.17")

    # Add module sources, compile defs
    modules_dir = src / "modules"
    for module in state.modules:
        mdir = modules_dir / module

        if not mdir.exists():
            mdir = modules_dir / "comm" / module

        if not mdir.exists():
            raise FileNotFoundError(f"Failed to find sources for module '{module}'")

        state.cmake.add_source_dir(mdir)
        transformed = module.upper().replace("-", "_")
        state.cmake.add_definition(f"GLOBED_MODULE_{transformed}", "1")

    # Link to bb if not oss build
    if state.oss_build:
        state.cmake.add_definition("GLOBED_OSS_BUILD", "1")
    else:
        if state.platform.is_windows():
            state.cmake.add_libraries("libs/bb/bb.lib", "ntdll.lib", "userenv.lib", "runtimeobject.lib", "Iphlpapi.lib", "bcrypt.lib")
        else:
            state.cmake.add_libraries(f"libs/bb/bb-{state.platform.platform_str(include_bit=True)}.a")


    ## Add cmake dependencies ##

    state.cmake.add_cpm_dep("capnproto", "capnproto/capnproto", "v1.2.0", {
        "CAPNP_LITE": "ON",
        "BUILD_TESTING": "OFF",
        "WITH_FIBERS": "OFF",
    }, link_name="CapnProto::capnp")
    state.cmake.add_cpm_dep("server-shared", "GlobedGD/server-shared", args.server_shared_ver, link_name="ServerShared")
    state.cmake.add_cpm_dep("qunet-cpp", "dankmeme01/qunet-cpp", args.qunet_ver, {
        "QUNET_QUIC_SUPPORT": "ON" if truthy(args.quic) else "OFF",
        "QUNET_ADVANCED_DNS": "ON" if truthy(args.advanced_dns) else "OFF",
        "QUNET_DEBUG": "ON" if state.debug else "OFF",
    }, link_name="qunet")
    state.cmake.add_cpm_dep("UIBuilder", "dankmeme01/uibuilder", "618ec98")
    state.cmake.add_cpm_dep("cue", "dankmeme01/cue", "2aaa7679")
    state.cmake.add_cpm_dep("argon", "GlobedGD/argon", "v1.2.0")
    state.cmake.add_cpm_dep("sinaps", "Prevter/sinaps", "2541d6d")
    state.cmake.add_cpm_dep("advanced_label", "Prevter/AdvancedLabel", "d78d7f82")

    if state.voice_support:
        state.cmake.add_cpm_dep("opus", "xiph/opus", "v1.5.2")
        state.cmake.add_definition("GLOBED_VOICE_SUPPORT", "1", "PUBLIC")

    # reexport some
    state.cmake.add_libraries("asp", "std23::nontype_functional", privacy="PUBLIC")

    state.silence_warnings_for("kj")
    state.silence_warnings_for("capnp")
    state.silence_warnings_for("libzstd_static")
    state.silence_warnings_for("opus")

    state.generate_mod_json()
    state.generate_cmake()

    print(f"Pre build script completed in {time.time() - start_time:.2f}s!")
