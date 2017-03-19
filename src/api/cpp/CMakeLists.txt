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

#set_source_files_properties(qot.c qot.h PROPERTIES LANGUAGE CXX )

FIND_PACKAGE(Threads REQUIRED)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
ADD_LIBRARY(qot_cpp SHARED qot.hpp qot.cpp lib/messenger.hpp lib/MessengerInterface.cpp lib/Messenger.cpp lib/Messenger.hpp 
	    lib/ClusterManager.cpp lib/ClusterManager.hpp lib/MsgingEntities.cpp lib/ClusterHandlers.cpp lib/ClusterHandlers.hpp ${OpenSplice_DATAMODEL})
TARGET_LINK_LIBRARIES(qot_cpp ${OpenSplice_LIBRARIES} ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
INSTALL(FILES qot.hpp DESTINATION include COMPONENT headers)
INSTALL(TARGETS qot_cpp DESTINATION lib COMPONENT libraries)

