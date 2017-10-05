# UDP Server Application which Timestamps incoming packets
ADD_EXECUTABLE(udpserver
	udpserver.c
)

# QoT UDP Server Application which Timestamps incoming packets
ADD_EXECUTABLE(udpserver-qot
	udpserver-qot.c
)
TARGET_LINK_LIBRARIES(udpserver-qot qot)

# UDP Client Application which periodically generates UDP datagrams
ADD_EXECUTABLE(udpclient
	udpclient.c
)

INSTALL(
	TARGETS 
		udpserver
	DESTINATION 
		bin 
	COMPONENT 
		applications
)

INSTALL(
	TARGETS 
		udpserver-qot
	DESTINATION 
		bin 
	COMPONENT 
		applications
)

INSTALL(
	TARGETS 
		udpclient
	DESTINATION 
		bin 
	COMPONENT 
		applications
)


