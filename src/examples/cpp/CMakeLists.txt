# Simple application to demonstrate how to connect to a timeline
ADD_EXECUTABLE(helloworld_cpp
 	helloworld.cpp
)
TARGET_LINK_LIBRARIES(helloworld_cpp qot_cpp)

# Robot Coordination edge node 
ADD_EXECUTABLE(edge_coordination
 	edge_coordination.cpp
)
TARGET_LINK_LIBRARIES(edge_coordination qot_cpp)

# Robot Coordination taskplanner node 
ADD_EXECUTABLE(taskplanner_coordination
 	taskplanner_coordination.cpp
)
TARGET_LINK_LIBRARIES(taskplanner_coordination qot_cpp)

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
		taskplanner_coordination
		edge_coordination
		#helloworld_capture 
		#helloworld_compare 
	DESTINATION 
		bin 
	COMPONENT 
		applications
)
