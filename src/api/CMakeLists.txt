# Create the qot user-space API
ADD_LIBRARY(qot SHARED
	qot.h
	qot.c
)

# Install the qot library to the given prefix
INSTALL(FILES qot.h DESTINATION include COMPONENT headers)
INSTALL(TARGETS qot DESTINATION lib COMPONENT libraries)