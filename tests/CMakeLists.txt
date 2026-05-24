include(GoogleTest)

set(_metadata_test_sources
    test_value.cpp
    test_object.cpp
    test_helpers.cpp
    test_path.cpp
    test_merge.cpp
    test_compare.cpp
    test_hash.cpp
    test_ostream.cpp
    test_format.cpp
)

if(METADATA_WITH_NLOHMANN_JSON AND TARGET nlohmann_json::nlohmann_json)
    list(APPEND _metadata_test_sources test_json.cpp)
endif()

if(METADATA_WITH_PARCEL AND TARGET parcel::parcel)
    list(APPEND _metadata_test_sources test_parcel.cpp)
endif()

add_executable(metadata_tests ${_metadata_test_sources})

target_link_libraries(metadata_tests
    PRIVATE
        metadata::metadata
        GTest::gtest_main
)

metadata_set_warnings(metadata_tests PRIVATE)
metadata_enable_sanitizers(metadata_tests)
metadata_enable_coverage(metadata_tests)

gtest_discover_tests(metadata_tests)
