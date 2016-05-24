# QoT Latency Daemon
ADD_EXECUTABLE(qotd
	qotd.c
)
TARGET_LINK_LIBRARIES(qotd qot -lrt -lm)

# Install the executables to the given prefix
INSTALL(
	TARGETS 
		qotd
	DESTINATION 
		bin 
	COMPONENT 
		applications
)


