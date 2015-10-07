# Create the qot user-space API
ADD_EXECUTABLE(helloworld 
	helloworld.c
)
TARGET_LINK_LIBRARIES(helloworld qot)

# Install the qot library to the given prefix
INSTALL(TARGETS helloworld DESTINATION bin COMPONENT applications)