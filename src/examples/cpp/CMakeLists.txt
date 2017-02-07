# Simple application to demonstrate how to connect to a timeline
ADD_EXECUTABLE(helloworld_cpp
 	helloworld.cpp
)
TARGET_LINK_LIBRARIES(helloworld_cpp qot_cpp)

# Simple application to demonstrate precise interrupt capture in global time
# ADD_EXECUTABLE(helloworld_capture 
# 	helloworld_capture.cpp
# )
# TARGET_LINK_LIBRARIES(helloworld_capture qot_cpp)

# Simple application to demonstrate precise interrupt generation in global time
# ADD_EXECUTABLE(helloworld_compare 
# 	helloworld_compare.cpp
# )
# TARGET_LINK_LIBRARIES(helloworld_compare qot_cpp)

# Install the qot library to the given prefix
INSTALL(
	TARGETS 
		helloworld_cpp
		#helloworld_capture 
		#helloworld_compare 
	DESTINATION 
		bin 
	COMPONENT 
		applications
)
