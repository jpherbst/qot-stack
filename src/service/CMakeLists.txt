# Add some helper CMake scripts
LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

#################################################################################################

# Configure cross compilation
IF (CROSS_COMPILE)

	# Required for cross-compiling
	SET(Boost_USE_STATIC_LIBS        OFF)
	SET(Boost_USE_STATIC_RUNTIME     OFF)
	SET(Boost_USE_MULTITHREADED      ON)
	SET(Boost_ALL_DYN_LINK           ON)  
	SET(Boost_NO_SYSTEM_PATHS        ON)

	# This is required for boost logging
	SET(BOOST_ROOT       "${CROSS_COMPILE_ROOTFS}/usr")
	SET(BOOST_INCLUDEDIR "${CROSS_COMPILE_ROOTFS}/usr/include")
	SET(BOOST_LIBRARYDIR "${CROSS_COMPILE_ROOTFS}/usr/lib/arm-linux-gnueabihf")

ENDIF (CROSS_COMPILE)

# This is required for boost::log
ADD_DEFINITIONS(-DBOOST_LOG_DYN_LINK)

# Preprocessor directive required to link boost trivial logging
FIND_PACKAGE(Boost 1.55 REQUIRED COMPONENTS thread system program_options log date_time)
FIND_PACKAGE(Threads REQUIRED)

# Location of header files for the entire project
INCLUDE_DIRECTORIES(
	"${Boost_INCLUDE_DIR}" 
)

# Location of boost libraries
LINK_DIRECTORIES(
	"${CROSS_COMPILE_ROOTFS}/usr/lib/arm-linux-gnueabihf"
	"${Boost_LIBRARY_DIRS}"
)

#################################################################################################

# Hint for finding opensplice
SET(ENV{OSPL_BASE} "${PROJECT_SOURCE_DIR}/thirdparty/opensplice/install/HDE/x86_64.linux-dev")
SET(ENV{OSPL_HOME} "${PROJECT_SOURCE_DIR}/thirdparty/opensplice/install/HDE/x86_64.linux-dev")

# Configure cross compilation
IF (CROSS_COMPILE)
	SET(ENV{OSPL_HOME} "${CROSS_COMPILE_ROOTFS}/opt/opensplice")
ENDIF (CROSS_COMPILE)

# Include the OpenSplice macros (IDL message compilation)
INCLUDE(MacroOpenSplice)

# Find the OpenSplice package
FIND_PACKAGE(OpenSplice REQUIRED)

# Location of header files for the entire project
INCLUDE_DIRECTORIES(
	${CMAKE_CURRENT_BINARY_DIR}
	${OpenSplice_INCLUDE_DIRS} 
)

# IDL MESSAGE COMPILATION
SET(idls msg/QoT.idl)
FOREACH(idl ${idls})
	OpenSplice_IDLGEN (${idl})
	SET(OpenSplice_DATAMODEL ${OpenSplice_DATAMODEL} ${outsources})
ENDFOREACH(idl)

#################################################################################################

# This is required for boost::log
ADD_DEFINITIONS(-DHAVE_ONESTEP_SYNC)
ADD_DEFINITIONS(-DHAVE_CLOCK_ADJTIME)
ADD_DEFINITIONS(-D_GNU_SOURCE)

# PTP library
ADD_LIBRARY(ptp SHARED
	linuxptp/bmc.c
	linuxptp/clock.c
	linuxptp/clockadj.c
	linuxptp/clockcheck.c
	linuxptp/config.c
	linuxptp/fault.c
	linuxptp/filter.c
	linuxptp/fsm.c
	linuxptp/linreg.c
	linuxptp/mave.c
	linuxptp/mmedian.c
	linuxptp/msg.c
	linuxptp/ntpshm.c
	linuxptp/phc.c
	linuxptp/pi.c
	linuxptp/port.c
	linuxptp/print.c
	linuxptp/raw.c
	linuxptp/servo.c
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
TARGET_LINK_LIBRARIES(ptp m)

# ptp4l app
ADD_EXECUTABLE(ptp4l 
	linuxptp/ptp4l.c
)
TARGET_LINK_LIBRARIES(ptp4l ptp)

#########################################################################################################

ADD_EXECUTABLE(qotdaemon 
	Synchronization.hpp
	Synchronization.cpp
	Coordinator.hpp
	Coordinator.cpp
	Timeline.cpp
	Timeline.hpp
	Notifier.cpp
	Notifier.hpp
	main.cpp
	${OpenSplice_DATAMODEL}
)
TARGET_LINK_LIBRARIES(qotdaemon ptp ${OpenSplice_LIBRARIES} ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

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
