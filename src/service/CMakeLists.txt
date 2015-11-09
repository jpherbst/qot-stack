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
FIND_PACKAGE(Boost 1.55 REQUIRED COMPONENTS thread system program_options log)
FIND_PACKAGE(Threads REQUIRED)

# Location of header files for the entire project
INCLUDE_DIRECTORIES(
	"${Boost_INCLUDE_DIR}" 
)

# Location of boost libraries
LINK_DIRECTORIES(
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
SET(idls msg/QoT.idl msg/TempControl.idl)
FOREACH(idl ${idls})
	OpenSplice_IDLGEN (${idl})
	SET(OpenSplice_DATAMODEL ${OpenSplice_DATAMODEL} ${outsources})
ENDFOREACH(idl)

#################################################################################################

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
	${OpenSplice_DATAMODEL}
)
TARGET_LINK_LIBRARIES(qotdaemon ${OpenSplice_LIBRARIES} ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

# Install the qot library to the given prefix
INSTALL(
	TARGETS 
		qotdaemon
	DESTINATION 
		bin 
	COMPONENT 
		applications
)
