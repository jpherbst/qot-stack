# led timing stuff
ADD_EXECUTABLE(ledapp
	ledapp.c
)
TARGET_LINK_LIBRARIES(ledapp qot)

# led testing
ADD_EXECUTABLE(ledtest
	ledtest.c
)
TARGET_LINK_LIBRARIES(ledtest qot)

# gpio tseting
ADD_EXECUTABLE(gpiotest
	gpiotest.c
)
TARGET_LINK_LIBRARIES(gpiotest qot)

INSTALL(
	TARGETS 
		ledapp
	DESTINATION 
		bin 
	COMPONENT 
		applications
)

INSTALL(
	TARGETS 
		ledtest
	DESTINATION 
		bin 
	COMPONENT 
		applications
)

INSTALL(
	TARGETS 
		gpiotest
	DESTINATION 
		bin 
	COMPONENT 
		applications
)
