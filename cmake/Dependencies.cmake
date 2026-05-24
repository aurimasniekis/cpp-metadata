include_guard(GLOBAL)
include(FetchContent)

if(METADATA_WITH_NLOHMANN_JSON)
    set(JSON_BuildTests OFF CACHE INTERNAL "")
    set(JSON_Install    OFF CACHE INTERNAL "")
    FetchContent_Declare(
        nlohmann_json
        URL      https://github.com/nlohmann/json/archive/refs/tags/v3.12.0.tar.gz
        URL_HASH SHA256=4b92eb0c06d10683f7447ce9406cb97cd4b453be18d7279320f7b2f025c10187
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        FIND_PACKAGE_ARGS 3.12.0
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif()

if(METADATA_WITH_PARCEL)
    set(PARCEL_BUILD_TESTS    OFF CACHE INTERNAL "")
    set(PARCEL_BUILD_EXAMPLES OFF CACHE INTERNAL "")
    set(PARCEL_INSTALL        OFF CACHE INTERNAL "")
    FetchContent_Declare(
        cpp-parcel
        URL https://github.com/aurimasniekis/cpp-parcel/archive/refs/tags/v0.1.0.tar.gz
        URL_HASH SHA256=d9d5550580c30cc30816448b986d1dcd8e0c267a8f98ad3b2d496d806d834020
        FIND_PACKAGE_ARGS 0.1.0 NAMES parcel
    )
    FetchContent_MakeAvailable(cpp-parcel)
endif()

if(METADATA_BUILD_TESTS)
    set(INSTALL_GTEST OFF CACHE INTERNAL "")
    set(BUILD_GMOCK   OFF CACHE INTERNAL "")
    FetchContent_Declare(
        googletest
        URL      https://github.com/google/googletest/archive/refs/tags/v1.17.0.tar.gz
        URL_HASH SHA256=65fab701d9829d38cb77c14acdc431d2108bfdbf8979e40eb8ae567edf10b27c
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        FIND_PACKAGE_ARGS NAMES GTest
    )
    FetchContent_MakeAvailable(googletest)
endif()
