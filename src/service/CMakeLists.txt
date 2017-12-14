# Add some helper CMake scripts
LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

#################################################################################################

# We need to change a few things to enable compilation
IF (CROSS_AM335X)
	SET(Boost_USE_STATIC_LIBS        OFF)
	SET(Boost_USE_STATIC_RUNTIME     OFF)
	SET(Boost_USE_MULTITHREADED      ON)
	SET(Boost_ALL_DYN_LINK           ON)
	SET(Boost_NO_SYSTEM_PATHS        ON)
	SET(BOOST_ROOT       "${CROSS_COMPILE_ROOTFS}/usr")
	SET(BOOST_INCLUDEDIR "${CROSS_COMPILE_ROOTFS}/usr/include")
	SET(BOOST_LIBRARYDIR "${CROSS_COMPILE_ROOTFS}/usr/lib/arm-linux-gnueabihf")
ENDIF (CROSS_AM335X)
IF (X86_64)
	SET(Boost_USE_STATIC_LIBS        OFF)
	SET(Boost_USE_STATIC_RUNTIME     OFF)
	SET(Boost_USE_MULTITHREADED      ON)
	SET(Boost_ALL_DYN_LINK           ON)
	SET(Boost_NO_SYSTEM_PATHS        ON)
	SET(BOOST_ROOT       "${CROSS_COMPILE_ROOTFS}/usr")
	SET(BOOST_INCLUDEDIR "${CROSS_COMPILE_ROOTFS}/usr/include")
	SET(BOOST_LIBRARYDIR "${CROSS_COMPILE_ROOTFS}/usr/lib/x86_64-linux-gnu")
ENDIF (X86_64)

# This is required for boost::log
ADD_DEFINITIONS(-DBOOST_LOG_DYN_LINK)

#debug symbols -sean
ADD_DEFINITIONS(-ggdb)

# Preprocessor directive required to link boost trivial logging
FIND_PACKAGE(Boost 1.55 REQUIRED
	COMPONENTS thread system program_options log date_time)
FIND_PACKAGE(Threads REQUIRED)

# Location of header files for the entire project
INCLUDE_DIRECTORIES(
	"${Boost_INCLUDE_DIR}"
)

# Location of boost libraries
IF (CROSS_AM335X)
	LINK_DIRECTORIES(
		"${CROSS_COMPILE_ROOTFS}/usr/lib/arm-linux-gnueabihf"
		"${Boost_LIBRARY_DIRS}"
	)
ENDIF (CROSS_AM335X)
IF (X86_64)
	LINK_DIRECTORIES(
		"${CROSS_COMPILE_ROOTFS}/usr/lib/x86_64-linux-gnu"
		"${Boost_LIBRARY_DIRS}"
	)
ENDIF (X86_64)


#################################################################################################

# Hint for finding opensplice
IF (CROSS_AM335X)
	SET(ENV{OSPL_BASE} "${PROJECT_SOURCE_DIR}/thirdparty/opensplice/install/HDE/armv7l.linux-dev")
	SET(ENV{OSPL_HOME} "${PROJECT_SOURCE_DIR}/thirdparty/opensplice/install/HDE/armv7l.linux-dev")
ENDIF (CROSS_AM335X)
IF (X86_64)
	SET(ENV{OSPL_BASE} "${PROJECT_SOURCE_DIR}/thirdparty/opensplice/install/HDE/x86_64.linux-dev")
	SET(ENV{OSPL_HOME} "${PROJECT_SOURCE_DIR}/thirdparty/opensplice/install/HDE/x86_64.linux-dev")
ENDIF (X86_64)

# Configure cross compilation
IF (CROSS_AM335X)
	SET(ENV{OSPL_HOME} "${CROSS_COMPILE_ROOTFS}/opt/opensplice")
ENDIF (CROSS_AM335X)

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
	sync/ptp/linuxptp/bmc.c
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
	sync/ptp/linuxptp/linregnew.c
	sync/ptp/linuxptp/port.c
	sync/ptp/linuxptp/print.c
	sync/ptp/linuxptp/raw.c
	sync/ptp/linuxptp/servo.c
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

# NTP library
ADD_LIBRARY(ntp SHARED
	sync/ntp/ntpv4/algorithm.c
	sync/ntp/ntpv4/ntpclient.c
	#sync/ntp/ntpv4/client.c
	#sync/ntp/ntpv4/server.c
)
TARGET_LINK_LIBRARIES(ntp m)

#########################################################################################################

ADD_EXECUTABLE(qotdaemon
	sync/ptp/PTP.hpp
	sync/ptp/PTP.cpp
	sync/ptp/uncertainty_data.h
	sync/ntp/NTP.hpp
	sync/ntp/NTP.cpp
	sync/ntp/uncertainty_data.h
	sync/Sync.hpp
	sync/Sync.cpp
        sync/SyncUncertainty.hpp
	sync/SyncUncertainty.cpp
	sync/ProbabilityLib.hpp
	sync/ProbabilityLib.cpp
	Coordinator.hpp
	Coordinator.cpp
	Timeline.cpp
	Timeline.hpp
	Notifier.cpp
	Notifier.hpp
	main.cpp
	${OpenSplice_DATAMODEL}
)
TARGET_LINK_LIBRARIES(qotdaemon ptp ntp ${OpenSplice_LIBRARIES} ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

ADD_EXECUTABLE(phc2sys
	sync/ptp/linuxptp/phc2sys.c
	sync/ptp/linuxptp/pmc_common.c
	sync/ptp/linuxptp/pmc_common.h
)
TARGET_LINK_LIBRARIES(phc2sys ptp ${CMAKE_THREAD_LIBS_INIT})

IF (NOT BUILD_VIRT_GUEST)
	ADD_EXECUTABLE(phc2phc
		phc2phc.c
	)
	TARGET_LINK_LIBRARIES(phc2phc ptp)

	ADD_EXECUTABLE(testptp
		testptp.c
	)
	TARGET_LINK_LIBRARIES(testptp -lrt)
ENDIF (NOT BUILD_VIRT_GUEST)

#########################################################################################################

# Install the PTP library
INSTALL(
	TARGETS
		ptp
	DESTINATION
		lib
	COMPONENT
		libraries
)

# Install the NTP library
INSTALL(
	TARGETS
		ntp
	DESTINATION
		lib
	COMPONENT
		libraries
)

# Install the qot daemon to the given prefix
INSTALL(
	TARGETS
		qotdaemon
	DESTINATION
		bin
	COMPONENT
		applications
)

# Install the phc2sys to the given prefix
INSTALL(
	TARGETS
		phc2sys
	DESTINATION
		bin
	COMPONENT
		applications
)


IF (NOT BUILD_VIRT_GUEST)
	# Install the phc2phc to the given prefix
	INSTALL(
		TARGETS
			phc2phc
		DESTINATION
			bin
		COMPONENT
			applications
	)

	# Install the testptp to the given prefix
	INSTALL(
		TARGETS
			testptp
		DESTINATION
			bin
		COMPONENT
			applications
)
ENDIF (NOT BUILD_VIRT_GUEST)

