# add directory for my apps
ADD_SUBDIRECTORY(sean)

# Simple application to demonstrate how to connect to a timeline
ADD_EXECUTABLE(helloworld
	helloworld.c
)
TARGET_LINK_LIBRARIES(helloworld qot)

ADD_EXECUTABLE(helloworld_capture
	helloworld_capture.c
)
TARGET_LINK_LIBRARIES(helloworld_capture qot)

ADD_EXECUTABLE(helloworld_compare
	helloworld_compare.c
)
TARGET_LINK_LIBRARIES(helloworld_compare qot)

ADD_EXECUTABLE(helloworld_gpio
	helloworld_gpio.c
)
TARGET_LINK_LIBRARIES(helloworld_gpio qot)

ADD_EXECUTABLE(helloworld_end2end
	helloworld_end2end.c
)
TARGET_LINK_LIBRARIES(helloworld_end2end qot)

#ADD_EXECUTABLE(helloworld_random
#	helloworld_random.c
#)
#TARGET_LINK_LIBRARIES(helloworld_random qot -lrt -lm)

ADD_EXECUTABLE(helloworld_gpio_vanilla
	helloworld_gpio_vanilla.c
)
TARGET_LINK_LIBRARIES(helloworld_gpio_vanilla -lrt -lm)

ADD_EXECUTABLE(helloworld_linux_test
	helloworld_linux_test.c
)
TARGET_LINK_LIBRARIES(helloworld_linux_test qot -lrt -lm)

ADD_EXECUTABLE(helloworld_tdma1
	helloworld_tdma1.c
)
TARGET_LINK_LIBRARIES(helloworld_tdma1 qot)

ADD_EXECUTABLE(helloworld_tdma2
	helloworld_tdma2.c
)
TARGET_LINK_LIBRARIES(helloworld_tdma2 qot)

ADD_EXECUTABLE(helloworld_timer
	helloworld_timer.c
)
TARGET_LINK_LIBRARIES(helloworld_timer qot)

ADD_EXECUTABLE(helloworld_capture_simple
	helloworld_capture_simple.c
)
TARGET_LINK_LIBRARIES(helloworld_capture_simple qot)

ADD_EXECUTABLE(helloworld_control_pwm
	helloworld_control_pwm.c
)
TARGET_LINK_LIBRARIES(helloworld_control_pwm qot)

#ADD_EXECUTABLE(helloworld_gpio_multiple
#	helloworld_gpio_multiple.c
#)
#TARGET_LINK_LIBRARIES(helloworld_gpio_multiple qot -lrt -lm)

#ADD_EXECUTABLE(helloworld_compare_demo
#	helloworld_compare_demo.c
#)
#TARGET_LINK_LIBRARIES(helloworld_compare_demo qot -lrt -lm)


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

INSTALL(
	TARGETS 
		helloworld_gpio
	DESTINATION 
		bin 
	COMPONENT 
		applications
)

INSTALL(
	TARGETS 
		helloworld_compare
	DESTINATION 
		bin 
	COMPONENT 
		applications
)

INSTALL(
	TARGETS 
		helloworld_end2end
	DESTINATION 
		bin 
	COMPONENT 
		applications
)

#INSTALL(
#	TARGETS 
#		helloworld_random
#	DESTINATION 
#		bin 
#	COMPONENT 
#		applications
#)

INSTALL(
	TARGETS 
		helloworld_gpio_vanilla
	DESTINATION 
		bin 
	COMPONENT 
		applications
)

INSTALL(
	TARGETS 
		helloworld_linux_test
	DESTINATION 
		bin 
	COMPONENT 
		applications
)

INSTALL(
	TARGETS 
		helloworld_tdma1
	DESTINATION 
		bin 
	COMPONENT 
		applications
)

INSTALL(
	TARGETS 
		helloworld_tdma2
	DESTINATION 
		bin 
	COMPONENT 
		applications
)

INSTALL(
	TARGETS 
		helloworld_timer
	DESTINATION 
		bin 
	COMPONENT 
		applications
)

INSTALL(
	TARGETS 
		helloworld_capture_simple
	DESTINATION 
		bin 
	COMPONENT 
		applications
)

INSTALL(
	TARGETS 
		helloworld_control_pwm
	DESTINATION 
		bin 
	COMPONENT 
		applications
)

#INSTALL(
#	TARGETS 
#		helloworld_gpio_multiple
#	DESTINATION 
#		bin 
#	COMPONENT 
#		applications
#)

#INSTALL(
#	TARGETS 
#		helloworld_compare_demo
#	DESTINATION 
#		bin 
#	COMPONENT 
#		applications
#)
