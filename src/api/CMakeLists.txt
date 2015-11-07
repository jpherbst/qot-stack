# Threading is needed for polling
FIND_PACKAGE(Threads REQUIRED)

# Create the qot user-space API
ADD_LIBRARY(qot SHARED
	qot.hpp
	qot.cpp
)
TARGET_LINK_LIBRARIES(qot ${CMAKE_THREAD_LIBS_INIT})

# Install the qot library to the given prefix
INSTALL(FILES qot.hpp DESTINATION include COMPONENT headers)
INSTALL(TARGETS qot DESTINATION lib COMPONENT libraries)