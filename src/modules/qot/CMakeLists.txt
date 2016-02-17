# Set the source files
SET(MODULE_SOURCE_FILES
    Kbuild
    qot_core.c
    qot_clock.c
    qot_clock_sysfs.c
    qot_timeline.c
    qot_timeline_chdev.c
    qot_timeline_sysfs.c
    qot_scheduler.c
    qot_admin.c
    qot_admin_chdev.c
    qot_admin_sysfs.c
    qot_user.c
    qot_user_chdev.c
)

# Set the source files
SET(MODULE_BUILD_PRODUCTS
    ${CMAKE_CURRENT_SOURCE_DIR}/.tmp_versions
    ${CMAKE_CURRENT_SOURCE_DIR}/.qot.o.cmd
    ${CMAKE_CURRENT_SOURCE_DIR}/.qot.ko.cmd
    ${CMAKE_CURRENT_SOURCE_DIR}/.qot.mod.o.cmd
    ${CMAKE_CURRENT_SOURCE_DIR}/.qot_core.o.cmd
    ${CMAKE_CURRENT_SOURCE_DIR}/.qot_core.o.cmd
    ${CMAKE_CURRENT_SOURCE_DIR}/.qot_clock.o.cmd
    ${CMAKE_CURRENT_SOURCE_DIR}/.qot_clock_sysfs.o.cmd
    ${CMAKE_CURRENT_SOURCE_DIR}/.qot_timeline.o.cmd
    ${CMAKE_CURRENT_SOURCE_DIR}/.qot_timeline_chdev.o.cmd
    ${CMAKE_CURRENT_SOURCE_DIR}/.qot_timeline_sysfs.o.cmd
    ${CMAKE_CURRENT_SOURCE_DIR}/.qot_scheduler.o.cmd
    ${CMAKE_CURRENT_SOURCE_DIR}/.qot_admin.o.cmd
    ${CMAKE_CURRENT_SOURCE_DIR}/.qot_admin_chdev.o.cmd
    ${CMAKE_CURRENT_SOURCE_DIR}/.qot_admin_sysfs.o.cmd
    ${CMAKE_CURRENT_SOURCE_DIR}/.qot_user.o.cmd
    ${CMAKE_CURRENT_SOURCE_DIR}/.qot_user_chdev.o.cmd
    ${CMAKE_CURRENT_SOURCE_DIR}/Module.symvers
    ${CMAKE_CURRENT_SOURCE_DIR}/modules.order
    ${CMAKE_CURRENT_SOURCE_DIR}/qot.mod.c
    ${CMAKE_CURRENT_SOURCE_DIR}/qot.mod.o
    ${CMAKE_CURRENT_SOURCE_DIR}/qot.o
    ${CMAKE_CURRENT_SOURCE_DIR}/qot_core.o
    ${CMAKE_CURRENT_SOURCE_DIR}/qot_clock.o
    ${CMAKE_CURRENT_SOURCE_DIR}/qot_clock_sysfs.o
    ${CMAKE_CURRENT_SOURCE_DIR}/qot_timeline.o
    ${CMAKE_CURRENT_SOURCE_DIR}/qot_timeline_chdev.o
    ${CMAKE_CURRENT_SOURCE_DIR}/qot_timeline_sysfs.o
    ${CMAKE_CURRENT_SOURCE_DIR}/qot_scheduler.o
    ${CMAKE_CURRENT_SOURCE_DIR}/qot_admin.o
    ${CMAKE_CURRENT_SOURCE_DIR}/qot_admin_chdev.o
    ${CMAKE_CURRENT_SOURCE_DIR}/qot_admin_sysfs.o
    ${CMAKE_CURRENT_SOURCE_DIR}/qot_user.o
    ${CMAKE_CURRENT_SOURCE_DIR}/qot_user_chdev.o
)

# Perform the compilation
SET(DRIVER_FILE qot.ko)

# Configure cross compilation
SET(KBUILD_CMD ${CMAKE_MAKE_PROGRAM} -C ${CROSS_KERNEL} M=${CMAKE_CURRENT_SOURCE_DIR} ARCH=${CMAKE_SYSTEM_PROCESSOR} CROSS_COMPILE=${CROSS_PREFIX} modules)

# Kernel build command
ADD_CUSTOM_COMMAND( OUTPUT  ${DRIVER_FILE}
                    COMMAND ${KBUILD_CMD}
                    DEPENDS ${MODULE_SOURCE_FILES} VERBATIM
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
ADD_CUSTOM_TARGET(ko_qot ALL DEPENDS ${DRIVER_FILE})
SET_DIRECTORY_PROPERTIES(PROPERTY ADDITIONAL_MAKE_CLEAN_FILES
    "${MODULE_BUILD_PRODUCTS}")