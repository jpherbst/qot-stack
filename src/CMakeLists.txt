# Minimum version of CMake required to build this file
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
PROJECT(misc)

# Find scripts
LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# DDS scripts
INCLUDE(${CMAKE_SOURCE_DIR}/cmake/dds_utils.cmake)

# Parameters
SET(CROSS_COMPILE 0)

# Extra parameters required for BBB
IF(CROSS_COMPILE)

	# Cross-compile properties
	SET(CMAKE_SYSTEM_NAME Linux)
	SET(CMAKE_SYSTEM_PROCESSOR arm) 
	SET(CMAKE_SYSTEM_VERSION 1)

	# Specify the cross compiler (need to work out )
	SET(CMAKE_C_COMPILER arm-linux-gnueabihf-gcc)
	SET(CMAKE_CXX_COMPILER arm-linux-gnueabihf-g++)
	SET(CMAKE_FIND_ROOT_PATH /export/rootfs)

	# Search for programs in the build host directories
	SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
	SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
	SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

	# Hints for finding boost in the rootfs
	SET(BOOST_INCLUDEDIR "/export/rootfs/usr/include")
	SET(BOOST_LIBRARYDIR "/export/rootfs/usr/lib/arm-linux-gnueabihf")

ENDIF(CROSS_COMPILE)

# We'll be using C++11 functionality in this project
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
SET(DDS_ROOT "/home/asymingt/Workspace/thirdparty/linuxptp/thirdparty/OpenDDS")
SET(ACE_ROOT "/home/asymingt/Workspace/thirdparty/linuxptp/thirdparty/OpenDDS/ACE_wrappers")

# Preprocessor directive required to link boost trivial logging
FIND_PACKAGE(Boost 1.55 REQUIRED COMPONENTS thread system program_options log)
FIND_PACKAGE(Threads REQUIRED)
FIND_PACKAGE(Math REQUIRED)
FIND_PACKAGE(DDS REQUIRED)

# This is required for boost logging
ADD_DEFINITIONS(-DBOOST_LOG_DYN_LINK)
ADD_DEFINITIONS(${DDS_DEFINITIONS})

# Location of header files for the entire project
INCLUDE_DIRECTORIES(
	${Boost_INCLUDE_DIR} 
	${DDS_INCLUDE_DIRS}
	${CMAKE_CURRENT_SOURCE_DIR} 
	${CMAKE_CURRENT_BINARY_DIR} 
)

# Location of boost libraries
LINK_DIRECTORIES(
	${Boost_LIBRARY_DIRS}
	${DDS_ROOT}/lib
	${ACE_ROOT}/lib 
)

# Extra parameters required for BBB
IF(CROSS_COMPILE)
	
	INCLUDE_DIRECTORIES(
		"/export/rootfs/usr/include/arm-linux-gnueabihf" 
		"/export/rootfs/usr/include"
	)
	
	LINK_DIRECTORIES(
		"/export/rootfs/usr/lib/arm-linux-gnueabihf" 
		"/export/rootfs/lib/arm-linux-gnueabihf"
	)

ENDIF(CROSS_COMPILE)

# IDL MESSAGE COMPILATION

COMPILE_IDL_FILES(${CMAKE_CURRENT_SOURCE_DIR}/messages/Timeline.idl)

# DEFAULT APPS

ADD_EXECUTABLE(pmc 
	linuxptp/msg.c 
	linuxptp/pmc.c 
	linuxptp/pmc_common.c 
	linuxptp/print.c 
	linuxptp/raw.c 
	linuxptp/sk.c 
	linuxptp/tlv.c 
	linuxptp/transport.c 
	linuxptp/udp.c 
	linuxptp/udp6.c 
	linuxptp/uds.c 
	linuxptp/util.c 
	linuxptp/version.c
)
TARGET_LINK_LIBRARIES(pmc ${MATH_LIBRARIES})

ADD_EXECUTABLE(phc2sys 
	linuxptp/clockadj.c 
	linuxptp/msg.c 
	linuxptp/phc.c 
	linuxptp/phc2sys.c 
	linuxptp/pi.c 
	linuxptp/pmc_common.c 
	linuxptp/print.c 
	linuxptp/servo.c
	linuxptp/raw.c 
	linuxptp/sk.c 
	linuxptp/stats.c 
	linuxptp/sysoff.c 
	linuxptp/tlv.c 
	linuxptp/transport.c 
	linuxptp/udp.c 
	linuxptp/udp6.c 
	linuxptp/uds.c 
	linuxptp/util.c
	linuxptp/version.c
)
TARGET_LINK_LIBRARIES(phc2sys ${MATH_LIBRARIES})

ADD_EXECUTABLE(hwstamp_ctl 
	linuxptp/hwstamp_ctl.c 
	linuxptp/version.c
)
TARGET_LINK_LIBRARIES(hwstamp_ctl ${MATH_LIBRARIES})

ADD_EXECUTABLE(ptp4l 
	linuxptp/bmc.c 
	linuxptp/bmc.h 
	linuxptp/clock.c 
	linuxptp/clock.h 
	linuxptp/clockadj.c
	linuxptp/clockadj.h 
	linuxptp/config.c
	linuxptp/config.h 
	linuxptp/fault.c
	linuxptp/fault.h 
	linuxptp/fsm.c
	linuxptp/fsm.h 
	linuxptp/mave.c
	linuxptp/mave.h
	linuxptp/msg.c
	linuxptp/msg.h 
	linuxptp/phc.c
	linuxptp/phc.h 
	linuxptp/pi.c 
	linuxptp/port.c 
	linuxptp/print.c 
	linuxptp/ptp4l.c 
	linuxptp/raw.c 
	linuxptp/servo.c 
	linuxptp/sk.c 
	linuxptp/stats.c 
	linuxptp/tlv.c 
	linuxptp/tmtab.c
	linuxptp/transport.c 
	linuxptp/udp.c 
	linuxptp/udp6.c 
	linuxptp/uds.c 
	linuxptp/util.c 
	linuxptp/version.c
)
TARGET_LINK_LIBRARIES(ptp4l ${MATH_LIBRARIES})

# Our apps

ADD_LIBRARY(ptp SHARED
	linuxptp/bmc.c 
	linuxptp/bmc.h 
	linuxptp/clock.c 
	linuxptp/clock.h 
	linuxptp/clockadj.c
	linuxptp/clockadj.h 
	linuxptp/config.c
	linuxptp/config.h 
	linuxptp/fault.c
	linuxptp/fault.h 
	linuxptp/fsm.c
	linuxptp/fsm.h 
	linuxptp/mave.c
	linuxptp/mave.h
	linuxptp/msg.c
	linuxptp/msg.h 
	linuxptp/phc.c
	linuxptp/phc.h 
	linuxptp/pi.c 
	linuxptp/port.c 
	linuxptp/print.c 
	linuxptp/raw.c 
	linuxptp/servo.c 
	linuxptp/sk.c 
	linuxptp/stats.c 
	linuxptp/tlv.c 
	linuxptp/tmtab.c
	linuxptp/transport.c 
	linuxptp/udp.c 
	linuxptp/udp6.c 
	linuxptp/uds.c 
	linuxptp/util.c 
	linuxptp/version.c
)
TARGET_LINK_LIBRARIES(ptp ${MATH_LIBRARIES})

ADD_EXECUTABLE(qotdaemon 
	Coordination.cpp
	Coordination.cpp
	Synchronization.cpp
	Synchronization.hpp
	Timeline.cpp
	Timeline.hpp
	Notifier.cpp
	Notifier.hpp
	main.cpp
	${ALL_IDL_SRCS}
)
TARGET_LINK_LIBRARIES(qotdaemon ptp ${Boost_LIBRARIES} ${DDS_LIBRARIES})