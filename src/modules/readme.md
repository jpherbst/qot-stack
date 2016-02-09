# Content description #

All source files in the root directory are compiled together to form the **qot_core** kernel module, which is the heart of the QoT stack. This code is divided into the following key modules:

1. **qot_core** - this is the base kernel module that initializes the modules below. It is kept separate, so that the individual core modules may be tested in isolation by the Google Test framework.
2. **qot_ioctl** - responsible for the maintenance of user space ioctl interfaces that act as a communication proxy between users and timelines (*/dev/qotusr*), as well as between root and the QoT system (*/dev/qotadm*).
3. **qot_scheduler** - responsible for modifying the Linux run queue, in order to provide an execution ordering that satisfies timeline needs.
4. **qot_sysfs** - provides root users with run-time introspection into the QoT stack through a SYSFS interface (*/sys/qot*).
5. **qot_timeline** - responsible for the creation and destruction of timelines, an tracks application bindings.