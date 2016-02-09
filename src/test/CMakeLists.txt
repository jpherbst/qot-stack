if (GTEST_FOUND)
    ENABLE_TESTING()
    INCLUDE_DIRECTORIES(${GTEST_INCLUDE_DIRS})
    ADD_EXECUTABLE(test_qot_math test_qot_math.cpp)
    TARGET_LINK_LIBRARIES(test_qot_math
        ${GTEST_LIBRARIES} ${GTEST_MAIN_LIBRARIES} pthread)
    ADD_TEST(TestQoTMath test_qot_math)
endif()
