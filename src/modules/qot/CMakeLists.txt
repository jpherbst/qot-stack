SET(MODULE_OUTPUT_FILES
    w00t.o
    w00t.mod.c
    w00t.mod.o
    w00t.ko
    .w00t.o.cmd
    .w00t.ko.cmd
    .w00t.mod.o.cmd
    .tmp_versions/w00t.mod
    .tmp_versions/
)
SET(MODULE_SOURCE_FILES Kbuild qot_core.c)
SET(DRIVER_FILE        qot.ko)
SET(DRIVER_TARGET_NAME qot-module)
SET(DRIVER_BIN_FILE    ${CMAKE_BINARY_DIR}/bin/${DRIVER_FILE})
SET(MODULE_SOURCE_DIR  ${PROJECT_SOURCE_DIR}/src/w00t)

SET( KERNEL_DIR "/lib/modules/${CMAKE_SYSTEM_VERSION}/build" )
SET( KBUILD_CMD ${CMAKE_MAKE_PROGRAM}
                -C ${KERNEL_DIR}
                M=${MODULE_SOURCE_DIR}
                modules )

ADD_CUSTOM_COMMAND( OUTPUT  ${DRIVER_BIN_FILE}
                            ${MODULE_OUTPUT_FILES}
                    COMMAND ${KBUILD_CMD}
                    COMMAND cp -f ${DRIVER_FILE} ${DRIVER_BIN_FILE}
                    WORKING_DIRECTORY ${MODULE_SOURCE_DIR}
                    DEPENDS ${MODULE_SOURCE_FILES} VERBATIM )

ADD_CUSTOM_TARGET ( ${DRIVER_TARGET_NAME}
                    ALL
                    DEPENDS ${DRIVER_BIN_FILE} )