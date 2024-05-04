function (generate_baked_resources_header JSON_FILE HEADER_FILE)
    # read the file
    file(READ ${JSON_FILE} JSON_CONTENT)

    # check md5
    string(MD5 JSON_HASH "${JSON_CONTENT}")

    # do not regenerate the file unless the json was changed
    if (EXISTS "${HEADER_FILE}" AND EXISTS "${HEADER_FILE}.md5")
        file(READ "${HEADER_FILE}.md5" JSON_FS_HASH)

        if (JSON_HASH STREQUAL JSON_FS_HASH)
            message(STATUS "Embedded resource hash matches, not regenerating")
            return()
        endif()
    endif()

    message(STATUS "Generating embedded resources header")
    file(WRITE "${HEADER_FILE}.md5" "${JSON_HASH}")

    string(JSON JSON_OBJECT GET ${JSON_CONTENT} strings)

    set(HEADER_CONTENT "#pragma once\n")
    string(APPEND HEADER_CONTENT "#include <util/adler32.hpp>\n")
    string(APPEND HEADER_CONTENT "namespace globed {\n")
    string(APPEND HEADER_CONTENT "constexpr inline const char* stringbyhash(uint32_t hash) {\n")
    string(APPEND HEADER_CONTENT "    switch (hash) {\n")

    # fucking kill me
    string(REGEX MATCHALL "\"([^\"]+)\"[ \t]*:" JSON_KEYS ${JSON_OBJECT})

    foreach(CKEY ${JSON_KEYS})
        string(REGEX REPLACE "\"([^\"]+)\"[ \t]*:" "\\1" KEY ${CKEY})

        string(JSON VALUE GET ${JSON_OBJECT} ${KEY})

        string(APPEND HEADER_CONTENT "        case util::crypto::adler32(\"${KEY}\"): return \"${VALUE}\";\n")
    endforeach()

    string(APPEND HEADER_CONTENT "        default: return nullptr;\n")
    string(APPEND HEADER_CONTENT "    }\n}\n}\n")

    file(WRITE "${HEADER_FILE}" "${HEADER_CONTENT}")
endfunction()
