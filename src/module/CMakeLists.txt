# Create the qot user-space API
ADD_LIBRARY(QoTclock SHARED
	uapi/QoTclock.h
	uapi/QoTclock.c
)

# Install the qot library to the given prefix
INSTALL(FILES qot.h DESTINATION include COMPONENT headers)
INSTALL(TARGETS QoTclock DESTINATION lib COMPONENT libraries)