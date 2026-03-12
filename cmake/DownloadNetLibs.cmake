# https://github.com/geode-sdk/geode/blob/main/loader/DownloadNetLibs.cmake

if (GEODE_TARGET_PLATFORM STREQUAL "iOS")
	set(net_libs_plat "ios")
	set(net_libs_hash "SHA256=1065ef2adc191a82de3f1d69826c29f8b1526accb092bde37966a5467606962c")
elseif (GEODE_TARGET_PLATFORM STREQUAL "MacOS")
	set(net_libs_plat "macos")
	set(net_libs_hash "SHA256=c91a60646b8b0b4d879edf1230006d430de72b84d8feee8151cbd9136241898e")
elseif (GEODE_TARGET_PLATFORM STREQUAL "Win64")
	set(net_libs_plat "windows")
	set(net_libs_hash "SHA256=478170d63fe6850e032e1e2de51ad744fa595aac40da5df65afe9996313bccb7")
elseif (GEODE_TARGET_PLATFORM STREQUAL "Android32")
	set(net_libs_plat "android32")
	set(net_libs_hash "SHA256=e113ffeab4bfcbc193977ae695b87e63cc8c1062dd20270321798e45ff2d350c")
elseif (GEODE_TARGET_PLATFORM STREQUAL "Android64")
	set(net_libs_plat "android64")
	set(net_libs_hash "SHA256=9ace6335c077d42e4bc4510feea35212d628dab54a4df49b6792d6ee8aa81909")
endif()

set(net_libs_version "8.19.0-9")
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

set(lib_names "c-ares" "crypto" "ssl" "ngtcp2_static" "ngtcp2_crypto_ossl_static" "libzstd_static")
set(lib_vars  "net_libs_cares" "net_libs_libcrypto" "net_libs_libssl" "net_libs_ngtcp2" "net_libs_ngtcp2_crypto_ossl" "net_libs_zstd")

foreach(target_name lib_path_var IN ZIP_LISTS lib_names lib_vars)
    if(NOT TARGET ${target_name})
        add_library(${target_name} STATIC IMPORTED GLOBAL)
        set_target_properties(${target_name} PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${net_libs_include}"
            IMPORTED_LOCATION "${${lib_path_var}}"
        )
    endif()
endforeach()

set_target_properties(ssl PROPERTIES INTERFACE_LINK_LIBRARIES "crypto")
set_target_properties(ngtcp2_crypto_ossl_static PROPERTIES INTERFACE_LINK_LIBRARIES "ngtcp2_static;ssl;crypto")
set(OPENSSL_ROOT_DIR "${net_libs_bin_SOURCE_DIR}" CACHE STRING "" FORCE)
set(OPENSSL_USE_STATIC_LIBS OFF CACHE STRING "" FORCE)
