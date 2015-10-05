# Add some helper CMake scripts
LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Include the OpenSplice macros (IDL message compilation)
INCLUDE(MacroOpenSplice)

# We'll be using C++11 functionality in this project
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

# Hint for finding opensplice
SET(ENV{OSPL_HOME} "${PROJECT_SOURCE_DIR}/thirdparty/opensplice/install/HDE/x86_64.linux-dev")

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

########################### OUR APPLICATION ##############################

# IDL MESSAGE COMPILATION

SET(idls msg/QoT.idl)
FOREACH(idl ${idls})
	OpenSplice_IDLGEN (${idl})
	message("${outsources}")
	SET(OpenSplice_DATAMODEL ${OpenSplice_DATAMODEL} ${outsources})
ENDFOREACH(idl)

ADD_LIBRARY(datamodel SHARED 
	${OpenSplice_DATAMODEL}
)

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
)
TARGET_LINK_LIBRARIES(qotdaemon datamodel ${Boost_LIBRARIES} ${OpenSplice_LIBRARIES})