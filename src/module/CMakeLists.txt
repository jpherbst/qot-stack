# Create the kernel module
ADD_CUSTOM_COMMAND(
	OUTPUT  qotsched.ko
	COMMAND ${CMAKE_MAKE_PROGRAM} -C =${CMAKE_CURRENT_SOURCE_DIR}/../kernel M=${CMAKE_CURRENT_SOURCE_DIR} modules
	WORKING_DIRECTORY ${CMAKE_CURRENT_BUILD_DIR}
	DEPENDS qotsched.c qotsched.h VERBATIM
)

# Create the qot user-space API
ADD_LIBRARY(qot SHARED
	qot.h
	qot.c
)

# Install the qot library to the given prefix
INSTALL(FILES qot.h DESTINATION include COMPONENT headers)
INSTALL(TARGETS qot DESTINATION lib COMPONENT libraries)