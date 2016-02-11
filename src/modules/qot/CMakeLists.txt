# Set the source files
SET(MODULE_SOURCE_FILES
    Kbuild
    qot_core.c
    qot_user_chdev.c
    qot_admin_chdev.c
    qot_admin_sysfs.c
)

# Set the source files
SET(MODULE_BUILD_PRODUCTS
    ${CMAKE_CURRENT_SOURCE_DIR}/.tmp_versions
    ${CMAKE_CURRENT_SOURCE_DIR}/.qot.o.cmd
    ${CMAKE_CURRENT_SOURCE_DIR}/.qot.ko.cmd
    ${CMAKE_CURRENT_SOURCE_DIR}/.qot.mod.o.cmd
    ${CMAKE_CURRENT_SOURCE_DIR}/.qot_user_chdev.o.cmd
    ${CMAKE_CURRENT_SOURCE_DIR}/.qot_admin_chdev.o.cmd
    ${CMAKE_CURRENT_SOURCE_DIR}/.qot_admin_sysfs.o.cmd
    ${CMAKE_CURRENT_SOURCE_DIR}/.qot_core.o.cmd
    ${CMAKE_CURRENT_SOURCE_DIR}/Module.symvers
    ${CMAKE_CURRENT_SOURCE_DIR}/modules.order
    ${CMAKE_CURRENT_SOURCE_DIR}/qot.ko
    ${CMAKE_CURRENT_SOURCE_DIR}/qot.mod.c
    ${CMAKE_CURRENT_SOURCE_DIR}/qot.mod.o
    ${CMAKE_CURRENT_SOURCE_DIR}/qot.o
    ${CMAKE_CURRENT_SOURCE_DIR}/qot_user_chdev.o
    ${CMAKE_CURRENT_SOURCE_DIR}/qot_admin_chdev.o
    ${CMAKE_CURRENT_SOURCE_DIR}/qot_admin_sysfs.o
    ${CMAKE_CURRENT_SOURCE_DIR}/qot_core.o
)

# Perform the compilation
SET(DRIVER_FILE qot.ko)

# Configure cross compilation
IF (CROSS_COMPILE)
    SET(KERNEL_DIR  /export/bb-kernel/KERNEL)
    SET(TOOL_CHAIN  /export/bb-kernel/dl/gcc-linaro-4.9-2015.02-3-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-)
    SET(KBUILD_CMD  ${CMAKE_MAKE_PROGRAM} -C ${KERNEL_DIR} M=${CMAKE_CURRENT_SOURCE_DIR} ARCH=arm CROSS_COMPILE=${TOOL_CHAIN} modules)
    SET(KSIGN_CMD   echo "Signing not required")
ELSE (CROSS_COMPILE)
    SET(KERNEL_DIR  /lib/modules/${CMAKE_SYSTEM_VERSION}/build)
    SET(KBUILD_CMD  ${CMAKE_MAKE_PROGRAM} -C ${KERNEL_DIR} M=${CMAKE_CURRENT_SOURCE_DIR} modules)
    SET(KSIGN_CMD   ${KERNEL_DIR}/scripts/sign-file sha512 kernel-signkey.priv kernel-signkey.x509 ${DRIVER_FILE})
ENDIF (CROSS_COMPILE)

# Kernel build command
ADD_CUSTOM_COMMAND( OUTPUT  ${DRIVER_FILE}
                    COMMAND ${KBUILD_CMD}
                    DEPENDS ${MODULE_SOURCE_FILES} VERBATIM
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                    COMMENT "Building qot kernel module...")
ADD_CUSTOM_TARGET(ko_qot ALL DEPENDS ${DRIVER_FILE})
SET_DIRECTORY_PROPERTIES(PROPERTY ADDITIONAL_MAKE_CLEAN_FILES
    "${MODULE_BUILD_PRODUCTS}")