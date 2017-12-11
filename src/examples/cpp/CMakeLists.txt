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

# Quad-Arm Robot Coordination Master
ADD_EXECUTABLE(quad_robot_master
 	quad-robot_coordinator.cpp
)
TARGET_LINK_LIBRARIES(quad_robot_master qot_cpp)

# Quad-Arm Robot Coordination Controller Type 1
ADD_EXECUTABLE(quad_robot1
 	quad-robot_coordination1.cpp
)
TARGET_LINK_LIBRARIES(quad_robot1 qot_cpp)

# Quad-Arm Robot Coordination Controller Type 2
ADD_EXECUTABLE(quad_robot2
 	quad-robot_coordination2.cpp
)
TARGET_LINK_LIBRARIES(quad_robot2 qot_cpp)

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
		quad_robot_master
		quad_robot1
		quad_robot2
		#helloworld_capture 
		#helloworld_compare 
	DESTINATION 
		bin 
	COMPONENT 
		applications
)
