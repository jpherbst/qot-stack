# Minimum version of CMake required to build this file
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
PROJECT(misc)

# Find scripts
LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# Include the OpenSplice macros (IDL message compilation)
INCLUDE(MacroOpenSplice)

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

# Hint for finding opensplice
SET(ENV{OSPL_HOME} "/home/asymingt/Workspace/thirdparty/linuxptp/thirdparty/opensplice/install/HDE/x86_64.linux-dev")

# Preprocessor directive required to link boost trivial logging
FIND_PACKAGE(Boost 1.55 REQUIRED COMPONENTS thread system program_options log)
FIND_PACKAGE(Threads REQUIRED)
FIND_PACKAGE(Math REQUIRED)
FIND_PACKAGE(OpenSplice REQUIRED)

# This is required for boost logging
ADD_DEFINITIONS(-DBOOST_LOG_DYN_LINK)

# Location of header files for the entire project
INCLUDE_DIRECTORIES(
	${Boost_INCLUDE_DIR} 
	${OpenSplice_INCLUDE_DIRS}
	${CMAKE_CURRENT_SOURCE_DIR} 
	${CMAKE_CURRENT_BINARY_DIR} 
)

# Location of boost libraries
LINK_DIRECTORIES(
	${Boost_LIBRARY_DIRS} 
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
set (idls QoT.idl)
foreach(idl ${idls})
	OpenSplice_IDLGEN (${idl})
	string (REGEX REPLACE "\(.*\).idl" "./gen/\\1.cpp" VARS_1 ${idl})
	string (REGEX REPLACE "\(.*\).idl" "./gen/\\1.h" VARS_2 ${idl})
	string (REGEX REPLACE "\(.*\).idl" "./gen/\\1Dcps.cpp" VARS_3 ${idl})
	string (REGEX REPLACE "\(.*\).idl" "./gen/\\1Dcps.h" VARS_4 ${idl})
	string (REGEX REPLACE "\(.*\).idl" "./gen/\\1Dcps_impl.cpp" VARS_5 ${idl})
	string (REGEX REPLACE "\(.*\).idl" "./gen/\\1Dcps_impl.h" VARS_6 ${idl})
	string (REGEX REPLACE "\(.*\).idl" "./gen/\\1SplDcps.cpp" VARS_7 ${idl})
	string (REGEX REPLACE "\(.*\).idl" "./gen/\\1SplDcps.h" VARS_8 ${idl})
	string (REGEX REPLACE "\(.*\).idl" "./gen/ccpp_\\1.h" VARS_9 ${idl})
	set(OpenSplice_DATAMODEL ${OpenSplice_DATAMODEL} ${VARS_1} ${VARS_2} ${VARS_3} ${VARS_4} ${VARS_5} ${VARS_6} ${VARS_7} ${VARS_8} ${VARS_9})
endforeach(idl)

# DEFAULT APPS

ADD_EXECUTABLE(pmc 
	src/linuxptp/msg.c 
	src/linuxptp/pmc.c 
	src/linuxptp/pmc_common.c 
	src/linuxptp/print.c 
	src/linuxptp/raw.c 
	src/linuxptp/sk.c 
	src/linuxptp/tlv.c 
	src/linuxptp/transport.c 
	src/linuxptp/udp.c 
	src/linuxptp/udp6.c 
	src/linuxptp/uds.c 
	src/linuxptp/util.c 
	src/linuxptp/version.c
)
TARGET_LINK_LIBRARIES(pmc ${MATH_LIBRARIES})

ADD_EXECUTABLE(phc2sys 
	src/linuxptp/clockadj.c 
	src/linuxptp/msg.c 
	src/linuxptp/phc.c 
	src/linuxptp/phc2sys.c 
	src/linuxptp/pi.c 
	src/linuxptp/pmc_common.c 
	src/linuxptp/print.c 
	src/linuxptp/servo.c
	src/linuxptp/raw.c 
	src/linuxptp/sk.c 
	src/linuxptp/stats.c 
	src/linuxptp/sysoff.c 
	src/linuxptp/tlv.c 
	src/linuxptp/transport.c 
	src/linuxptp/udp.c 
	src/linuxptp/udp6.c 
	src/linuxptp/uds.c 
	src/linuxptp/util.c
	src/linuxptp/version.c
)
TARGET_LINK_LIBRARIES(phc2sys ${MATH_LIBRARIES})

ADD_EXECUTABLE(hwstamp_ctl 
	src/linuxptp/hwstamp_ctl.c 
	src/linuxptp/version.c
)
TARGET_LINK_LIBRARIES(hwstamp_ctl ${MATH_LIBRARIES})

ADD_EXECUTABLE(ptp4l 
	src/linuxptp/bmc.c 
	src/linuxptp/bmc.h 
	src/linuxptp/clock.c 
	src/linuxptp/clock.h 
	src/linuxptp/clockadj.c
	src/linuxptp/clockadj.h 
	src/linuxptp/config.c
	src/linuxptp/config.h 
	src/linuxptp/fault.c
	src/linuxptp/fault.h 
	src/linuxptp/fsm.c
	src/linuxptp/fsm.h 
	src/linuxptp/mave.c
	src/linuxptp/mave.h
	src/linuxptp/msg.c
	src/linuxptp/msg.h 
	src/linuxptp/phc.c
	src/linuxptp/phc.h 
	src/linuxptp/pi.c 
	src/linuxptp/port.c 
	src/linuxptp/print.c 
	src/linuxptp/ptp4l.c 
	src/linuxptp/raw.c 
	src/linuxptp/servo.c 
	src/linuxptp/sk.c 
	src/linuxptp/stats.c 
	src/linuxptp/tlv.c 
	src/linuxptp/tmtab.c
	src/linuxptp/transport.c 
	src/linuxptp/udp.c 
	src/linuxptp/udp6.c 
	src/linuxptp/uds.c 
	src/linuxptp/util.c 
	src/linuxptp/version.c
)
TARGET_LINK_LIBRARIES(ptp4l ${MATH_LIBRARIES})

# Our apps

ADD_LIBRARY(ptp SHARED
	src/linuxptp/bmc.c 
	src/linuxptp/bmc.h 
	src/linuxptp/clock.c 
	src/linuxptp/clock.h 
	src/linuxptp/clockadj.c
	src/linuxptp/clockadj.h 
	src/linuxptp/config.c
	src/linuxptp/config.h 
	src/linuxptp/fault.c
	src/linuxptp/fault.h 
	src/linuxptp/fsm.c
	src/linuxptp/fsm.h 
	src/linuxptp/mave.c
	src/linuxptp/mave.h
	src/linuxptp/msg.c
	src/linuxptp/msg.h 
	src/linuxptp/phc.c
	src/linuxptp/phc.h 
	src/linuxptp/pi.c 
	src/linuxptp/port.c 
	src/linuxptp/print.c 
	src/linuxptp/raw.c 
	src/linuxptp/servo.c 
	src/linuxptp/sk.c 
	src/linuxptp/stats.c 
	src/linuxptp/tlv.c 
	src/linuxptp/tmtab.c
	src/linuxptp/transport.c 
	src/linuxptp/udp.c 
	src/linuxptp/udp6.c 
	src/linuxptp/uds.c 
	src/linuxptp/util.c 
	src/linuxptp/version.c
)
TARGET_LINK_LIBRARIES(ptp ${MATH_LIBRARIES})

ADD_LIBRARY(datamodel SHARED ${OpenSplice_DATAMODEL})

ADD_EXECUTABLE(qotdaemon 
	src/Coordination.cpp
	src/Coordination.cpp
	src/Synchronization.cpp
	src/Synchronization.hpp
	src/Timeline.cpp
	src/Timeline.hpp
	src/Notifier.cpp
	src/Notifier.hpp
	src/main.cpp
)
TARGET_LINK_LIBRARIES(qotdaemon datamodel ${Boost_LIBRARIES} ${OpenSplice_LIBRARIES})