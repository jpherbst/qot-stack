# @file CMakeLists.txt
# @brief Base CMake build script for the qot-stack project
# @author Andrew Symington
#
# Copyright (c) Regents of the University of California, 2015.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 	1. Redistributions of source code must retain the above copyright notice,
#     this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright notice,
#     this list of conditions and the following disclaimer in the documentation
#     and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

# Locate Google Test framework
FIND_PACKAGE(GTest REQUIRED)

# Everything in our system must know about qot_types.h
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

# Build the kernel modules
OPTION(BUILD_MODULES "Build kernel modules" ON)
IF (BUILD_MODULES)
	ADD_SUBDIRECTORY(modules)
ENDIF (BUILD_MODULES)

# Build the programmer interface
OPTION(BUILD_PROGAPI "Build programmer interface" OFF)
IF (BUILD_PROGAPI)
	ADD_SUBDIRECTORY(api)
ENDIF (BUILD_PROGAPI)

# Build the GPS Service
OPTION(BUILD_GPS_SERVICE "Build GPS service" OFF)
IF (BUILD_GPS_SERVICE)
	ADD_SUBDIRECTORY(gps-service)
ENDIF (BUILD_GPS_SERVICE)

# Build the qot latency daemon
OPTION(BUILD_QOTLATD "Build qot latency daemon" OFF)
IF (BUILD_QOTLATD)
	ADD_SUBDIRECTORY(qot-daemon)
ENDIF (BUILD_QOTLATD)

# Build the system service
OPTION(BUILD_SERVICE "Build system service" OFF)
IF (BUILD_SERVICE)
	ADD_SUBDIRECTORY(service)
ENDIF (BUILD_SERVICE)

# Build example applications
OPTION(BUILD_EXAMPLE "Build example applications" OFF)
IF (BUILD_EXAMPLE)
	ADD_SUBDIRECTORY(examples)
ENDIF (BUILD_EXAMPLE)

# Build example applications
OPTION(BUILD_UTILS "Build utility applications" OFF)
IF (BUILD_UTILS)
	ADD_SUBDIRECTORY(utils)
ENDIF (BUILD_UTILS)

# Build Virtdaemon for Host
OPTION(BUILD_VIRT "Build QEMU-KVM virtualization functionality" OFF)
IF (BUILD_VIRT)
	ADD_SUBDIRECTORY(virt)
ENDIF (BUILD_VIRT)

# Build API with Paravirtual extensions for QEMU-KVM Linux Guest (Build Virt Should also be turned on)
option(BUILD_VIRT_GUEST "Build API with Paravirtual extensions for QEMU-KVM Linux Guest" OFF)
IF (BUILD_VIRT_GUEST)
  add_definitions(-DPARAVIRT_GUEST)
ENDIF (BUILD_VIRT_GUEST)

# Build the unit tests (doesn't make sense for cross-compilation)
IF (NOT CROSS_AM335X)
	OPTION(BUILD_UNITEST "Build unit tests" OFF)
	IF (BUILD_UNITEST)
		ADD_SUBDIRECTORY(test)
	ENDIF (BUILD_UNITEST)
ENDIF (NOT CROSS_AM335X)


