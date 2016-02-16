# Set the source files
SET(MODULE_SOURCE_FILES
    Kbuild
    qot_chronos.c
)

# Set the source files
SET(MODULE_BUILD_PRODUCTS
    ${CMAKE_CURRENT_SOURCE_DIR}/.tmp_versions
    ${CMAKE_CURRENT_SOURCE_DIR}/.qot_chronos.o.cmd
    ${CMAKE_CURRENT_SOURCE_DIR}/.qot_chronos.ko.cmd
    ${CMAKE_CURRENT_SOURCE_DIR}/.qot_chronos.mod.o.cmd
    ${CMAKE_CURRENT_SOURCE_DIR}/Module.symvers
    ${CMAKE_CURRENT_SOURCE_DIR}/modules.order
    ${CMAKE_CURRENT_SOURCE_DIR}/qot_chronos.ko
    ${CMAKE_CURRENT_SOURCE_DIR}/qot_chronos.mod.c
    ${CMAKE_CURRENT_SOURCE_DIR}/qot_chronos.mod.o
    ${CMAKE_CURRENT_SOURCE_DIR}/qot_chronos.o
)

# Perform the compilation
SET(DRIVER_FILE qot_chronos.ko)

# Configure cross compilation
SET(KBUILD_CMD  ${CMAKE_MAKE_PROGRAM} -C ${CROSS_KERNEL} M=${CMAKE_CURRENT_SOURCE_DIR} ARCH=${CMAKE_SYSTEM_PROCESSOR} CROSS_COMPILE=${CROSS_PREFIX} modules)

# Kernel build command
ADD_CUSTOM_COMMAND( OUTPUT  ${DRIVER_FILE}
                    COMMAND ${KBUILD_CMD}
                    DEPENDS ${MODULE_SOURCE_FILES} VERBATIM
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
ADD_CUSTOM_TARGET(ko_qot_chronos ALL DEPENDS ${DRIVER_FILE})
SET_DIRECTORY_PROPERTIES(PROPERTY ADDITIONAL_MAKE_CLEAN_FILES
    "${MODULE_BUILD_PRODUCTS}")