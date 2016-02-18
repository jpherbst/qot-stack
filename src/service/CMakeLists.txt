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


# Add some helper CMake scripts
LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# FIND BOOST ###################################################################

# Configure cross compilation for boost
IF (CROSS_COMPILE)
	SET(Boost_USE_STATIC_LIBS        OFF)
	SET(Boost_USE_STATIC_RUNTIME     OFF)
	SET(Boost_USE_MULTITHREADED      ON)
	SET(Boost_ALL_DYN_LINK           ON)
	SET(Boost_NO_SYSTEM_PATHS        ON)
	SET(BOOST_ROOT       "${CROSS_COMPILE_ROOTFS}/usr")
	SET(BOOST_INCLUDEDIR "${CROSS_COMPILE_ROOTFS}/usr/include")
	SET(BOOST_LIBRARYDIR "${CROSS_COMPILE_ROOTFS}/usr/lib/arm-linux-gnueabihf")
ENDIF (CROSS_COMPILE)

# This is required for boost::log
ADD_DEFINITIONS(-DBOOST_LOG_DYN_LINK)

# Preprocessor directive required to link boost trivial logging
FIND_PACKAGE(Boost 1.54 REQUIRED
	COMPONENTS thread system program_options log date_time)
FIND_PACKAGE(Threads REQUIRED)

# Location of header files for the entire project
INCLUDE_DIRECTORIES("${Boost_INCLUDE_DIR}")

# Location of boost libraries
LINK_DIRECTORIES("${Boost_LIBRARY_DIRS}")

# FIND OPENSPLICE ##############################################################

# Hint for finding opensplice
SET(ENV{OSPL_BASE}
	"${PROJECT_SOURCE_DIR}/thirdparty/opensplice/install/HDE/x86_64.linux-dev")
SET(ENV{OSPL_HOME}
	"${PROJECT_SOURCE_DIR}/thirdparty/opensplice/install/HDE/x86_64.linux-dev")

# Configure cross compilation
IF (CROSS_COMPILE)
	SET(ENV{OSPL_HOME} "${CROSS_COMPILE_ROOTFS}/opt/opensplice")
ENDIF (CROSS_COMPILE)

# Include the OpenSplice macros (IDL message compilation)
#INCLUDE(MacroOpenSplice)

# Find the OpenSplice package
#FIND_PACKAGE(OpenSplice REQUIRED)

# Location of header files for the entire project
INCLUDE_DIRECTORIES(
	${CMAKE_CURRENT_BINARY_DIR}
	${OpenSplice_INCLUDE_DIRS}
)

# IDL MESSAGE COMPILATION
#SET(idls msg/QoT.idl)
#FOREACH(idl ${idls})
#	OpenSplice_IDLGEN (${idl})
#	SET(OpenSplice_DATAMODEL ${OpenSplice_DATAMODEL} ${outsources})
#ENDFOREACH(idl)

#################################################################################################

# This is required for building linuxptp
ADD_DEFINITIONS(-DHAVE_ONESTEP_SYNC)
ADD_DEFINITIONS(-DHAVE_CLOCK_ADJTIME)
ADD_DEFINITIONS(-D_GNU_SOURCE)

# PTP library
ADD_LIBRARY(ptp SHARED
	sync/ptp/linuxptp/bmc.c
        sync/ptp/linuxptp/servo.c
	sync/ptp/linuxptp/clock.c
	sync/ptp/linuxptp/clockadj.c
	sync/ptp/linuxptp/clockcheck.c
	sync/ptp/linuxptp/config.c
	sync/ptp/linuxptp/fault.c
	sync/ptp/linuxptp/filter.c
	sync/ptp/linuxptp/fsm.c
	sync/ptp/linuxptp/linreg.c
	sync/ptp/linuxptp/mave.c
	sync/ptp/linuxptp/mmedian.c
	sync/ptp/linuxptp/msg.c
	sync/ptp/linuxptp/ntpshm.c
	sync/ptp/linuxptp/phc.c
	sync/ptp/linuxptp/pi.c
	sync/ptp/linuxptp/port.c
	sync/ptp/linuxptp/print.c
	sync/ptp/linuxptp/raw.c
	sync/ptp/linuxptp/sk.c
	sync/ptp/linuxptp/stats.c
	sync/ptp/linuxptp/sysoff.c
	sync/ptp/linuxptp/tlv.c
	sync/ptp/linuxptp/transport.c
	sync/ptp/linuxptp/udp.c
	sync/ptp/linuxptp/udp6.c
	sync/ptp/linuxptp/uds.c
	sync/ptp/linuxptp/util.c
	sync/ptp/linuxptp/version.c
	sync/ptp/linuxptp/ptp.c
)
TARGET_LINK_LIBRARIES(ptp m)

################################################################################

ADD_EXECUTABLE(qotdaemon 
	sync/ptp/PTP.hpp
	sync/ptp/PTP.cpp
	sync/Sync.hpp
	sync/Sync.cpp
#	Synchronization.hpp
#	Synchronization.cpp
	Coordinator.hpp
	Coordinator.cpp
	Timeline.cpp
	Timeline.hpp
	Notifier.cpp
	Notifier.hpp
	main.cpp
	${OpenSplice_DATAMODEL}
)
TARGET_LINK_LIBRARIES(qotdaemon
	ptp ${OpenSplice_LIBRARIES} ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

# Install the PTP library
INSTALL(
	TARGETS
		ptp
	DESTINATION
		lib
	COMPONENT
		libraries
)

# Install the qot library to the given prefix
INSTALL(
	TARGETS
		qotdaemon
	DESTINATION
		bin
	COMPONENT
		applications
)
