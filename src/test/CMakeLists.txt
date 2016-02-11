if (GTEST_FOUND)

    INCLUDE_DIRECTORIES(${GTEST_INCLUDE_DIRS})

    ADD_EXECUTABLE(test_qot_math test_qot_math.cpp)
    TARGET_LINK_LIBRARIES(test_qot_math
        ${GTEST_LIBRARIES} ${GTEST_MAIN_LIBRARIES} pthread)
    ADD_TEST(TestQoTMath test_qot_math)

    ADD_EXECUTABLE(test_qot_chardev_usr test_qot_chardev_usr.cpp)
    TARGET_LINK_LIBRARIES(test_qot_chardev_usr
        ${GTEST_LIBRARIES} ${GTEST_MAIN_LIBRARIES} pthread)
    ADD_TEST(TestQoTChardevUsr test_qot_chardev_usr)

    ADD_EXECUTABLE(test_qot_chardev_adm test_qot_chardev_adm.cpp)
    TARGET_LINK_LIBRARIES(test_qot_chardev_adm
        ${GTEST_LIBRARIES} ${GTEST_MAIN_LIBRARIES} pthread)
    ADD_TEST(TestQoTChardevAdm test_qot_chardev_adm)

endif()
