# QoT Latency Daemon
ADD_EXECUTABLE(qot_virtd
	qot_virthost.c
)
TARGET_LINK_LIBRARIES(qot_virtd -lrt -lm)

# Install the executables to the given prefix
INSTALL(
	TARGETS 
		qot_virtd
	DESTINATION 
		bin 
	COMPONENT 
		applications
)

