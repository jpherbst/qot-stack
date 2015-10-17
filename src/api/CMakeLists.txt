# Create the qot user-space API
ADD_LIBRARY(qot SHARED
	qot.hpp
	qot.cpp
)

# Install the qot library to the given prefix
INSTALL(FILES qot.hpp DESTINATION include COMPONENT headers)
INSTALL(TARGETS qot DESTINATION lib COMPONENT libraries)