# Minimum version of CMake required to build this file
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
PROJECT(misc)

# Helper scripts
LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

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

# This is required for boost logging
ADD_DEFINITIONS(-DBOOST_LOG_DYN_LINK)

# Preprocessor directive required to link boost trivial logging
FIND_PACKAGE(Boost 1.55 REQUIRED COMPONENTS thread system program_options log)
FIND_PACKAGE(Threads REQUIRED)
FIND_PACKAGE(Math REQUIRED)

# Location of header files for the entire project
INCLUDE_DIRECTORIES(
	${Boost_INCLUDE_DIR} 
	${CMAKE_CURRENT_SOURCE_DIR} 
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

ADD_EXECUTABLE(pmc 
	msg.c 
	pmc.c 
	pmc_common.c 
	print.c 
	raw.c 
	sk.c 
	tlv.c 
	transport.c 
	udp.c 
	udp6.c 
	uds.c 
	util.c 
	version.c
)
TARGET_LINK_LIBRARIES(pmc ${MATH_LIBRARIES})

ADD_EXECUTABLE(phc2sys 
	clockadj.c 
	msg.c 
	phc.c 
	phc2sys.c 
	pi.c 
	pmc_common.c 
	print.c 
	servo.c
 	raw.c 
 	sk.c 
 	stats.c 
 	sysoff.c 
 	tlv.c 
 	transport.c 
 	udp.c 
 	udp6.c 
 	uds.c 
 	util.c
 	version.c
)
TARGET_LINK_LIBRARIES(phc2sys ${MATH_LIBRARIES})

ADD_EXECUTABLE(hwstamp_ctl 
	hwstamp_ctl.c 
	version.c
)
TARGET_LINK_LIBRARIES(hwstamp_ctl ${MATH_LIBRARIES})

ADD_EXECUTABLE(ptp4l 
	bmc.c 
	clock.c 
	clockadj.c 
	config.c 
	fault.c 
	fsm.c 
	ptp4l.c 
	mave.c
 	msg.c 
 	phc.c 
 	pi.c 
 	port.c 
 	print.c 
 	raw.c 
 	servo.c 
 	sk.c 
 	stats.c 
 	tlv.c 
 	tmtab.c
 	transport.c 
 	udp.c 
 	udp6.c 
 	uds.c 
 	util.c 
 	version.c
)
TARGET_LINK_LIBRARIES(ptp4l ${MATH_LIBRARIES})


ADD_EXECUTABLE(qotdaemon 
	new/Timeline.cpp
	new/Timeline.hpp
	new/Notifier.cpp
	new/Notifier.hpp
	new/main.cpp
)
TARGET_LINK_LIBRARIES(qotdaemon ${MATH_LIBRARIES} ${Boost_LIBRARIES} )