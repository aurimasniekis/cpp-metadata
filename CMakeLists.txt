cmake_minimum_required(VERSION 3.25)

project(metadata
    VERSION     0.1.0
    DESCRIPTION "Modern C++23 header-only metadata container (JSON-like)"
    LANGUAGES   CXX
)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD          23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
        Debug Release RelWithDebInfo MinSizeRel)
endif()

option(METADATA_BUILD_TESTS         "Build metadata tests"                ${PROJECT_IS_TOP_LEVEL})
option(METADATA_BUILD_EXAMPLES      "Build metadata examples"             ${PROJECT_IS_TOP_LEVEL})
option(METADATA_BUILD_DOCS          "Build Doxygen API documentation"     OFF)
option(METADATA_ENABLE_CLANG_TIDY   "Run clang-tidy during build"         OFF)
option(METADATA_ENABLE_SANITIZERS   "Build with ASan + UBSan (Debug)"     OFF)
option(METADATA_ENABLE_COVERAGE     "Build with Clang source-based coverage" OFF)
option(METADATA_WARNINGS_AS_ERRORS  "Treat compiler warnings as errors"   ${PROJECT_IS_TOP_LEVEL})
option(METADATA_INSTALL             "Generate install rules for metadata" ${PROJECT_IS_TOP_LEVEL})
option(METADATA_WITH_NLOHMANN_JSON  "Enable optional nlohmann/json integration" ON)
option(METADATA_WITH_PARCEL         "Enable optional cpp-parcel integration"    OFF)

include(CompilerWarnings)
include(Sanitizers)
include(Coverage)
include(Dependencies)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/include/md/version.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/include/md/version.hpp"
    @ONLY
)

add_library(metadata INTERFACE)
add_library(metadata::metadata ALIAS metadata)

target_include_directories(metadata
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(metadata INTERFACE cxx_std_23)

if(METADATA_WITH_NLOHMANN_JSON AND TARGET nlohmann_json::nlohmann_json)
    target_link_libraries(metadata INTERFACE nlohmann_json::nlohmann_json)
    target_compile_definitions(metadata INTERFACE METADATA_WITH_NLOHMANN_JSON=1)
endif()

if(METADATA_WITH_PARCEL AND TARGET parcel::parcel)
    target_link_libraries(metadata INTERFACE parcel::parcel)
    target_compile_definitions(metadata INTERFACE METADATA_WITH_PARCEL=1)
endif()

if(METADATA_ENABLE_CLANG_TIDY)
    find_program(METADATA_CLANG_TIDY_EXE NAMES clang-tidy REQUIRED)
    set(_metadata_tidy_cmd
        "${METADATA_CLANG_TIDY_EXE}"
        "--config-file=${CMAKE_CURRENT_SOURCE_DIR}/.clang-tidy"
    )
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        execute_process(
            COMMAND "${CMAKE_CXX_COMPILER}" -print-search-dirs
            OUTPUT_VARIABLE _metadata_gcc_search_dirs
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        if(_metadata_gcc_search_dirs MATCHES "install: ([^\n]+)")
            list(APPEND _metadata_tidy_cmd
                "--extra-arg-before=--gcc-install-dir=${CMAKE_MATCH_1}")
            message(STATUS
                "metadata: pinning clang-tidy to gcc install dir ${CMAKE_MATCH_1}")
        endif()
    endif()
    set(CMAKE_CXX_CLANG_TIDY "${_metadata_tidy_cmd}")
    message(STATUS "metadata: clang-tidy enabled (${METADATA_CLANG_TIDY_EXE})")
endif()

if(METADATA_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(METADATA_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(METADATA_BUILD_DOCS)
    find_package(Doxygen REQUIRED)
    set(METADATA_DOXYGEN_OUTPUT_DIR "${CMAKE_BINARY_DIR}/docs")

    # Pull nlohmann_json / parcel headers onto Doxygen's INCLUDE_PATH so its
    # preprocessor can resolve `__has_include(<nlohmann/json.hpp>)` and
    # `<parcel/parcel.h>` in <md/json.hpp> and <md/parcel.hpp>. Without this,
    # those headers are scanned but the inherited types are invisible and
    # members get dropped under HIDE_UNDOC_MEMBERS.
    set(METADATA_DOXYGEN_INCLUDE_PATH "")
    if(METADATA_WITH_NLOHMANN_JSON AND DEFINED nlohmann_json_SOURCE_DIR)
        list(APPEND METADATA_DOXYGEN_INCLUDE_PATH
            "${nlohmann_json_SOURCE_DIR}/include")
    endif()
    if(METADATA_WITH_PARCEL AND DEFINED cpp-parcel_SOURCE_DIR)
        list(APPEND METADATA_DOXYGEN_INCLUDE_PATH
            "${cpp-parcel_SOURCE_DIR}/include")
    endif()
    list(JOIN METADATA_DOXYGEN_INCLUDE_PATH " " METADATA_DOXYGEN_INCLUDE_PATH)

    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in"
        "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile"
        @ONLY
    )
    add_custom_target(metadata_docs
        COMMAND ${DOXYGEN_EXECUTABLE} "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT "Generating Doxygen documentation"
        VERBATIM
    )
endif()

if(METADATA_INSTALL AND DEFINED nlohmann_json_SOURCE_DIR)
    message(STATUS
        "metadata: METADATA_INSTALL disabled — nlohmann_json was fetched and "
        "cannot be re-exported. Install it via system package or find_package, "
        "or disable METADATA_WITH_NLOHMANN_JSON, to re-enable install rules."
    )
    set(METADATA_INSTALL OFF)
endif()

if(METADATA_INSTALL)
    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)

    install(TARGETS metadata
        EXPORT  metadataTargets
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    install(DIRECTORY include/metadata
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.hpp"
    )

    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/include/md/version.hpp"
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/metadata
    )

    install(EXPORT metadataTargets
        FILE        metadataTargets.cmake
        NAMESPACE   metadata::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/metadata
    )

    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/metadataConfigVersion.cmake"
        VERSION       ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
        ARCH_INDEPENDENT
    )

    configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/metadataConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/metadataConfig.cmake"
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/metadata
    )

    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/metadataConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/metadataConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/metadata
    )
endif()
