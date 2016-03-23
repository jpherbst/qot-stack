# Simple application to demonstrate how to connect to a timeline
ADD_EXECUTABLE(helloworld
	helloworld.c
)
TARGET_LINK_LIBRARIES(helloworld qot)

# Install the qot library to the given prefix
INSTALL(
	TARGETS 
		helloworld
	DESTINATION 
		bin 
	COMPONENT 
		applications
)