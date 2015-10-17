# Simple application to demonstrate how to connect to a timeline
ADD_EXECUTABLE(helloworld_timeline 
	helloworld_timeline.cpp
)
TARGET_LINK_LIBRARIES(helloworld_timeline qot)

# Simple application to demonstrate precise interrupt capture in global time
ADD_EXECUTABLE(helloworld_capture 
	helloworld_capture.cpp
)
TARGET_LINK_LIBRARIES(helloworld_capture qot)

# Simple application to demonstrate precise interrupt generation in global time
ADD_EXECUTABLE(helloworld_compare 
	helloworld_compare.cpp
)
TARGET_LINK_LIBRARIES(helloworld_compare qot)

# Simple application to demonstrate how to wait until a global time
ADD_EXECUTABLE(helloworld_waituntil
	helloworld_waituntil.cpp
)
TARGET_LINK_LIBRARIES(helloworld_waituntil qot)

# Install the qot library to the given prefix
INSTALL(
	TARGETS 
		helloworld_timeline 
		helloworld_capture 
		helloworld_compare 
		helloworld_waituntil 
	DESTINATION 
		bin 
	COMPONENT 
		applications
)
