# Add some helper CMake scripts
LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

FIND_PACKAGE(Threads REQUIRED)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

# This is required for boost::log
ADD_DEFINITIONS(-DHAVE_ONESTEP_SYNC)
ADD_DEFINITIONS(-DHAVE_CLOCK_ADJTIME)
ADD_DEFINITIONS(-D_GNU_SOURCE)

# PTP library
ADD_LIBRARY(ptpforgps SHARED
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
	linuxptp/linregnew.c
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
	linuxptp/ptp.c
)
TARGET_LINK_LIBRARIES(ptpforgps m)

#########################################################################################################


ADD_EXECUTABLE(gpsync
	gps.h
	gps.c
	gps_sync.c
)
TARGET_LINK_LIBRARIES(gpsync ptpforgps qot)

#########################################################################################################

# Install the PTP library
INSTALL(
	TARGETS
		ptpforgps
	DESTINATION
		lib
	COMPONENT
		libraries
)

# Install the gpsync to the given prefix
INSTALL(
	TARGETS
		gpsync
	DESTINATION
		bin
	COMPONENT
		applications
)

