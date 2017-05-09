# QoT Virtualization (Host) Daemon
IF (NOT BUILD_VIRT_GUEST)
	ADD_EXECUTABLE(qot_virtd
		qot_virthost.c
	)
	TARGET_LINK_LIBRARIES(qot_virtd -lrt -lm -lpthread)

	# Install the executables to the given prefix
	INSTALL(
		TARGETS 
			qot_virtd
		DESTINATION 
			bin 
		COMPONENT 
			applications
	)
ENDIF (NOT BUILD_VIRT_GUEST)


