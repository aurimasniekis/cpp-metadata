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
    # cpp-parcel v0.2.0 links commons::commons INTERFACE. It declares cpp-commons
    # in its own cmake/Dependencies.cmake (via include(Dependencies)), but that
    # include never runs in our build: metadata's cmake dir sits ahead of
    # parcel's on CMAKE_MODULE_PATH, so parcel's include(Dependencies) resolves
    # back to *this* file — already fired by include_guard(GLOBAL), hence a
    # no-op. We therefore declare cpp-commons here (mirroring parcel's pin) so
    # commons::commons exists before parcel's target_link_libraries runs.
    FetchContent_Declare(
        cpp-commons
        URL      https://github.com/aurimasniekis/cpp-commons/archive/refs/tags/v0.1.3.tar.gz
        URL_HASH SHA256=2f5615ac96a1a1dddda5424ed75c0d1a0142f115f215502562f479ef138fc30d
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        FIND_PACKAGE_ARGS 0.1 NAMES commons
    )
    FetchContent_MakeAvailable(cpp-commons)

    set(PARCEL_BUILD_TESTS    OFF CACHE INTERNAL "")
    set(PARCEL_BUILD_EXAMPLES OFF CACHE INTERNAL "")
    set(PARCEL_INSTALL        OFF CACHE INTERNAL "")
    FetchContent_Declare(
        cpp-parcel
        URL https://github.com/aurimasniekis/cpp-parcel/archive/refs/tags/v0.2.0.tar.gz
        URL_HASH SHA256=5040c50fcd0ad001be761a35166a89a1792be667fbc436747a5ddbc8a377fe6a
        FIND_PACKAGE_ARGS 0.2.0 NAMES parcel
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
