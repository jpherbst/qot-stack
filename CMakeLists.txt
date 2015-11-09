# @file CMakeLists.txt
# @brief Base build script for the qot-stack project
# @author Andrew Symington
# 
# Copyright (c) Regents of the University of California, 2015. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification, 
# are permitted provided that the following conditions are met:
# 	1. Redistributions of source code must retain the above copyright notice, 
#     this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright notice, 
#     this list of conditions and the following disclaimer in the documentation
#     and/or other materials provided with the distribution.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
# IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
# OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# Minimum version of CMake required to build this file
CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

# Name and version of this project
PROJECT(qot-stack VERSION 0.1.0.0)

# Basic options
OPTION(CROSS_COMPILE "Cross-compile for arm-linux-gnueabihf" ON)

# Configure cross compilation
IF (CROSS_COMPILE)

	# User-definable root filesystem location
	SET(CROSS_COMPILE_ROOTFS "/export/rootfs" CACHE STRING "Path to ROOTFS")

	# User-definable root filesystem location
	SET(CMAKE_INSTALL_PREFIX "${CROSS_COMPILE_ROOTFS}/usr/local")

	# System, architecture and version
	SET(CMAKE_SYSTEM_NAME Linux)
	SET(CMAKE_SYSTEM_PROCESSOR arm) 
	SET(CMAKE_SYSTEM_VERSION 1)
	#SET(CMAKE_SYSROOT "${CROSS_COMPILE_ROOTFS}")

	# Specify the cross compiler
	SET(CMAKE_C_COMPILER arm-linux-gnueabihf-gcc)
	SET(CMAKE_CXX_COMPILER arm-linux-gnueabihf-g++)

	# Location of boost libraries
	LINK_DIRECTORIES(
		"/usr/arm-linux-gnueabihf/lib"
	)

	# Where is the target environment 
	SET(CMAKE_FIND_ROOT_PATH ${CROSS_COMPILE_ROOTFS})
	SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
	SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
	SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
	SET(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)


ENDIF (CROSS_COMPILE)
	
# Platform-wide C and C++ standard
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99")

# Include the code
ADD_SUBDIRECTORY(src)





