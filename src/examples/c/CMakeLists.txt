# Simple application to demonstrate how to connect to a timeline
ADD_EXECUTABLE(helloworld
	helloworld.c
)
TARGET_LINK_LIBRARIES(helloworld qot)

ADD_EXECUTABLE(helloworld_capture
	helloworld_capture.c
)
TARGET_LINK_LIBRARIES(helloworld_capture qot)

# Install the executables to the given prefix
INSTALL(
	TARGETS 
		helloworld
	DESTINATION 
		bin 
	COMPONENT 
		applications
)

INSTALL(
	TARGETS 
		helloworld_capture
	DESTINATION 
		bin 
	COMPONENT 
		applications
)