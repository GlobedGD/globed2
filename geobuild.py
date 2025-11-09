"""
Globed pre-build script.
This script is invoked by cmake when configuring the project, you'll find that
most of the build configuration is done here, rather than CMakeLists.txt.

This file ends up generating the mod.json and a .cmake file that gets included by the main CMakeLists.txt.
For more details, see https://github.com/dankmeme01/geobuild/
"""

from typing import TYPE_CHECKING
if TYPE_CHECKING:
    from .build.geobuild.prelude import *

from dataclasses import dataclass

# minimum required geode, can be a commit or a tag
REQUIRED_GEODE_VERSION = "4bb9f16"
QUNET_VERSION = "68dd378"
SERVER_SHARED_VERSION = "f9670e3"

@dataclass
class State:
    build: Build
    debug: bool
    release: bool
    oss: bool
    voice: bool
    modules: list[str]
    server_url: str


def print_info(state: State):
    build = state.build
    config = build.config

    print(f"========== Globed build configuration ==========")
    print(f"Platform: {build.platform.name}, host: {build.config.host_desc()}, debug: {state.debug}, release: {state.release}, OSS build: {state.oss}")
    print(f"Voice: {state.voice}, modules: {state.modules}, server URL: '{state.server_url}'")
    print(f"Compiler: {config.compiler_id} {config.compiler_version}, frontend: '{config.compiler_frontend}'")
    print("=================================================")

def main(build: Build):
    config = build.config

    build.set_cache_variable("GLOBED_MODULES", "", desc="Enabled modules (comma-separated)")
    build.set_cache_variable("GLOBED_DEFAULT_MAIN_SERVER_URL", "qunet://globed.dev", desc="Default main server URL")

    quic = build.add_option("GLOBED_QUIC_SUPPORT", True, desc="Enable QUIC support")
    advanced_dns = build.add_option("GLOBED_ADVANCED_DNS_SUPPORT", True, desc="Enable advanced DNS support")
    debug = build.add_option("GLOBED_DEBUG", desc="Enable debug mode")
    release = build.add_option("GLOBED_RELEASE", desc="Enable release mode optimizations")
    oss = build.add_option("GLOBED_OSS_BUILD", desc="Open source build that does not require closed-source dependencies")
    voice = build.add_option("GLOBED_VOICE_SUPPORT", True, desc="Enable voice chat support")

    state = State(
        build=build,
        debug=debug,
        release=release,
        oss=oss,
        voice=voice,
        modules=[m.strip() for m in config.var("GLOBED_MODULES", "").split(",") if m.strip()],
        server_url=config.var("GLOBED_DEFAULT_MAIN_SERVER_URL", "qunet://globed.dev")
    )

    print_info(state)

    # check some preconditions, such as needing geode nightly or regular clang
    if config.is_clang_cl or not config.is_clang:
        fatal_error("Clang-cl and MSVC are not supported, Globed can only be built with Clang")

    build.verify_sdk_at_least(REQUIRED_GEODE_VERSION)

    # Add necessary modules
    state.modules.append("deathlink")
    state.modules.append("two-player")
    state.modules.append("ui")

    # Add base sources
    src = config.project_dir / "src"
    build.add_source_dir(src / "core")
    build.add_source_dir(src / "audio")
    build.add_source_dir(src / "util")
    build.add_source_dir(src / "ui")
    build.add_source_dir(src / "soft-link")
    build.add_source_dir(src / "platform" / config.platform.platform_str())

    # Include dirs
    build.add_include_dir(config.project_dir / "include", Privacy.PUBLIC)
    build.add_include_dir(config.project_dir / "src")
    build.add_include_dir(config.project_dir / "libs")

    # Compile definitions / variables
    if state.release:
        build.enable_lto()

    build.add_definition("GLOBED_BUILD")
    build.add_definition("GLOBED_DEFAULT_MAIN_SERVER_URL", f'"{state.server_url}"')
    build.add_definition("UIBUILDER_NO_ARROW", "1")

    if state.debug:
        build.add_definition("GLOBED_DEBUG", "1")
        build.set_variable("QUNET_DEBUG", "ON")

    if state.voice:
        build.add_definition("GLOBED_VOICE_SUPPORT", "1")
        if config.platform.is_windows():
            build.add_definition("GLOBED_VOICE_CAN_TALK", "1")

    # Add geode dependencies
    build.enable_mod_json_generation("mod.json.template")

    build.add_geode_dep("geode.node-ids", ">=v1.10.0")

    if 'scripting-ui' in state.modules:
        build.add_geode_dep("alphalaneous.editortab_api", ">=1.0.17")

    # Add module sources, compile defs
    modules_dir = src / "modules"
    for module in state.modules:
        mdir = modules_dir / module

        if not mdir.exists():
            mdir = modules_dir / "comm" / module

        if not mdir.exists():
            raise FileNotFoundError(f"Failed to find sources for module '{module}'")

        build.add_source_dir(mdir)
        transformed = module.upper().replace("-", "_")
        build.add_definition(f"GLOBED_MODULE_{transformed}", "1")

    # Link to bb if not oss build
    if state.oss:
        build.add_definition("GLOBED_OSS_BUILD", "1")
    else:
        if config.platform.is_windows():
            build.link_libraries("libs/bb/bb.lib", "ntdll.lib", "userenv.lib", "runtimeobject.lib", "Iphlpapi.lib", "bcrypt.lib")
        else:
            build.link_library(f"libs/bb/bb-{config.platform.platform_str(include_bit=True)}.a")


    ## Add cmake dependencies ##

    build.add_cpm_dep("capnproto/capnproto", "v1.2.0", {
        "CAPNP_LITE": "ON",
        "BUILD_TESTING": "OFF",
        "WITH_FIBERS": "OFF",
    }, link_name="CapnProto::capnp")
    build.add_cpm_dep("GlobedGD/server-shared", SERVER_SHARED_VERSION, link_name="ServerShared")
    build.add_cpm_dep("dankmeme01/qunet-cpp", QUNET_VERSION, {
        "QUNET_QUIC_SUPPORT": "ON" if quic else "OFF",
        "QUNET_ADVANCED_DNS": "ON" if advanced_dns else "OFF",
        "QUNET_DEBUG": "ON" if state.debug else "OFF",
    }, link_name="qunet")
    build.add_cpm_dep("dankmeme01/uibuilder", "618ec98", link_name="UIBuilder")
    build.add_cpm_dep("dankmeme01/cue", "778140ea")
    build.add_cpm_dep("GlobedGD/argon", "v1.2.0")
    build.add_cpm_dep("Prevter/sinaps", "2541d6d")
    build.add_cpm_dep("Prevter/AdvancedLabel", "d78d7f82", link_name="advanced_label")

    if state.voice:
        build.add_cpm_dep("xiph/opus", "v1.5.2", {
            "OPUS_INSTALL_PKG_CONFIG_MODULE": "OFF",
        })
        build.add_definition("GLOBED_VOICE_SUPPORT", "1", Privacy.PUBLIC)

    # reexport some
    build.link_libraries("asp", "std23::nontype_functional", privacy=Privacy.PUBLIC)

    build.silence_warnings_for("kj")
    build.silence_warnings_for("capnp")
    build.silence_warnings_for("libzstd_static")
    build.silence_warnings_for("opus")

    # check dep updates in debug
    if state.debug:
        build.check_for_updates()
