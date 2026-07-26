include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.15.2
)
FetchContent_MakeAvailable(googletest)

add_executable(celebrant_tests smoke_test.cpp order_book_test.cpp property_test.cpp codec_test.cpp)
target_link_libraries(celebrant_tests PRIVATE GTest::gtest_main celebrant)

include(GoogleTest)
gtest_discover_tests(celebrant_tests)
