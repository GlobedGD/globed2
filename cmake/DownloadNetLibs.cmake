# https://github.com/geode-sdk/geode/blob/main/loader/DownloadNetLibs.cmake

option(GLOBED_USE_WOLFSSL "" ON)

if (GEODE_TARGET_PLATFORM STREQUAL "iOS")
	set(net_libs_plat "ios")
	set(net_libs_hash "SHA256=cae7b36070b4cbb859dc1bf6cfda9d1bf08b4dfaf5520d9d4f2f37509941e4fa")
elseif (GEODE_TARGET_PLATFORM STREQUAL "MacOS")
	set(net_libs_plat "macos")
	set(net_libs_hash "SHA256=7e7ed5ab12bfca549e2e357e20ecb8ebc8f0f5e42baaca7ba044299ff133a408")
elseif (GEODE_TARGET_PLATFORM STREQUAL "Win64")
	set(net_libs_plat "windows")
	set(net_libs_hash "SHA256=9943b91054eed91e809c2837480bedd1de341df7b56299ffafc56399273b9bbb")
elseif (GEODE_TARGET_PLATFORM STREQUAL "Android32")
	set(net_libs_plat "android32")
	set(net_libs_hash "SHA256=cccd5785845b43e7d0c695c5d459e2ed8c07e24ecae4c4eff6a41cfb0f8377ce")
elseif (GEODE_TARGET_PLATFORM STREQUAL "Android64")
	set(net_libs_plat "android64")
	set(net_libs_hash "SHA256=28b81463de403fd153b307a9fe4cbf3d41b7f6dc14011bf98d9cd7e96e79eba5")
endif()

set(net_libs_version "8.19.0-10")
CPMAddPackage(
	NAME net_libs_bin
	VERSION "${net_libs_version}_${net_libs_plat}"
	URL "https://github.com/geode-sdk/net_libs/releases/download/v${net_libs_version}/curl-${net_libs_plat}.zip"
	URL_HASH ${net_libs_hash}
	DOWNLOAD_ONLY YES
)

set(net_libs_include ${net_libs_bin_SOURCE_DIR}/include)
if (WIN32)
    set(net_libs_cares ${net_libs_bin_SOURCE_DIR}/cares.lib)
    set(net_libs_libcrypto ${net_libs_bin_SOURCE_DIR}/libcrypto.lib)
    set(net_libs_libssl ${net_libs_bin_SOURCE_DIR}/libssl.lib)
    set(net_libs_ngtcp2 ${net_libs_bin_SOURCE_DIR}/ngtcp2.lib)
    set(net_libs_ngtcp2_crypto_ossl ${net_libs_bin_SOURCE_DIR}/ngtcp2_crypto_ossl.lib)
    set(net_libs_zstd ${net_libs_bin_SOURCE_DIR}/zstd_static.lib)
else()
    set(net_libs_cares ${net_libs_bin_SOURCE_DIR}/libcares.a)
    set(net_libs_libcrypto ${net_libs_bin_SOURCE_DIR}/libcrypto.a)
    set(net_libs_libssl ${net_libs_bin_SOURCE_DIR}/libssl.a)
    set(net_libs_ngtcp2 ${net_libs_bin_SOURCE_DIR}/libngtcp2.a)
    set(net_libs_ngtcp2_crypto_ossl ${net_libs_bin_SOURCE_DIR}/libngtcp2_crypto_ossl.a)
    set(net_libs_zstd ${net_libs_bin_SOURCE_DIR}/libzstd.a)
endif()

set(lib_names "c-ares" "crypto" "ssl" "libzstd_static")
set(lib_vars  "net_libs_cares" "net_libs_libcrypto" "net_libs_libssl" "net_libs_zstd")

if (GLOBED_USE_WOLFSSL)
    CPMAddPackage(
        URI "gh:wolfSSL/wolfssl@5.8.4-stable"
        OPTIONS "WOLFSSL_QUIC ON"
                "WOLFSSL_HARDEN OFF"
                "WOLFSSL_OLD_TLS OFF"
                "WOLFSSL_TLSV12 OFF"
                "WOLFSSL_SESSION_TICKET ON"
                "WOLFSSL_INSTALL OFF"
                "WOLFSSL_CRYPT_TESTS OFF"
                "WOLFSSL_EXAMPLES OFF"
                "WOLFSSL_ALT_CERT_CHAINS ON" # needed for tls, not sure why /shrug
                "BUILD_SHARED_LIBS OFF"
    )

    get_target_property(WOLFSSL_INCLUDE_DIRS2 wolfssl INCLUDE_DIRECTORIES)
    set(WOLFSSL_INCLUDE_DIRS ${WOLFSSL_INCLUDE_DIRS2} CACHE STRING "" FORCE)

    if (MSVC)
        target_compile_options(wolfssl PRIVATE /w)
    else()
        target_compile_options(wolfssl PRIVATE -w)
    endif()
else()
    list(APPEND lib_names "ngtcp2_crypto_ossl_static")
    list(APPEND lib_names "ngtcp2_static")
    list(APPEND lib_vars "net_libs_ngtcp2_crypto_ossl")
    list(APPEND lib_vars "net_libs_ngtcp2")
endif()

foreach(target_name lib_path_var IN ZIP_LISTS lib_names lib_vars)
    if(NOT TARGET ${target_name})
        add_library(${target_name} STATIC IMPORTED GLOBAL)
        set_target_properties(${target_name} PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${net_libs_include}"
            IMPORTED_LOCATION "${${lib_path_var}}"
        )
    endif()
endforeach()

if (GLOBED_USE_WOLFSSL)
else()
    set_target_properties(ssl PROPERTIES INTERFACE_LINK_LIBRARIES "crypto")
    set_target_properties(ngtcp2_crypto_ossl_static PROPERTIES INTERFACE_LINK_LIBRARIES "ngtcp2_static;ssl;crypto")
    set(OPENSSL_ROOT_DIR "${net_libs_bin_SOURCE_DIR}" CACHE STRING "" FORCE)
    set(OPENSSL_USE_STATIC_LIBS OFF CACHE STRING "" FORCE)
endif()
