# Set the source files
SET(MODULE_SOURCE_FILES
    Kbuild
    qot_core.c
    qot_timeline.c
    qot_scheduler.c
    qot_sysfs.c
    qot_ioctl.c
)

# Perform the compilation
SET(DRIVER_FILE qot.ko)
SET(KERNEL_DIR  /export/bb-kernel/KERNEL)
SET(CROSS_COMP   /export/bb-kernel/dl/gcc-linaro-4.9-2015.02-3-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-)
SET(KBUILD_CMD  ${CMAKE_MAKE_PROGRAM} -C ${KERNEL_DIR} M=${CMAKE_CURRENT_SOURCE_DIR} ARCH=arm CROSS_COMPILE=${CROSS_COMP} modules)

# Kernel build command
ADD_CUSTOM_COMMAND( OUTPUT  ${DRIVER_FILE}
                    COMMAND ${KBUILD_CMD}
                    DEPENDS ${MODULE_SOURCE_FILES} VERBATIM
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                    COMMENT "Building qot kernel module...")
ADD_CUSTOM_TARGET(ko_qot ALL DEPENDS ${DRIVER_FILE})