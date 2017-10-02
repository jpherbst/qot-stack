# UDP Server Application which Timestamps incoming packets
ADD_EXECUTABLE(clockbench
	ClockBench.cpp
)
TARGET_LINK_LIBRARIES(clockbench qot)

INSTALL(
	TARGETS 
		clockbench
	DESTINATION 
		bin 
	COMPONENT 
		applications
)



