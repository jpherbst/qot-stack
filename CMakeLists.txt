# Minimum version of CMake required to build this file
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

# Name and version of this project
PROJECT(qot-stack)

# Cross-compile options for the BeagleBone Black
OPTION(CROSS_AM335X "Build for the am335x" OFF)
IF (CROSS_AM335X)
    SET(CROSS_KERNEL /export/bb-kernel/KERNEL
    	CACHE STRING "Path to target kernel source")
    SET(CROSS_PREFIX /export/bb-kernel/dl/gcc-linaro-4.9-2015.02-3-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-
    	CACHE STRING "Path to target cross compiler")
	SET(CROSS_COMPILE_ROOTFS "/export/rootfs"
		CACHE STRING "Path to target rootfs")
	SET(CMAKE_INSTALL_PREFIX "${CROSS_COMPILE_ROOTFS}/usr/local")
	SET(CMAKE_SYSTEM_NAME Linux)
	SET(CMAKE_SYSTEM_PROCESSOR arm)
	SET(CMAKE_SYSTEM_VERSION 1)
	SET(CMAKE_C_COMPILER ${CROSS_PREFIX}-gcc)
	SET(CMAKE_CXX_COMPILER ${CROSS_PREFIX}-g++)
	LINK_DIRECTORIES(
		"/usr/arm-linux-gnueabihf/lib"
		"/usr/lib/arm-linux-gnueabihf"
	)
	SET(CMAKE_FIND_ROOT_PATH ${CROSS_COMPILE_ROOTFS})
	SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
	SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
	SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
	SET(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
ELSE (CROSS_AM335X)
	SET(CROSS_KERNEL /lib/modules/${CMAKE_SYSTEM_VERSION}/build
    	CACHE STRING "Path to target kernel source")
	SET(CROSS_PREFIX ""
		CACHE STRING "Path to target cross compiler")
	SET(CROSS_COMPILE_ROOTFS ""
		CACHE STRING "Path to target rootfs")
	ENABLE_TESTING()
ENDIF (CROSS_AM335X)

# Platform-wide C and C++ options
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99")

# Uninstall target
CONFIGURE_FILE(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)
ADD_CUSTOM_TARGET(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)

# All source code
ADD_SUBDIRECTORY(src)