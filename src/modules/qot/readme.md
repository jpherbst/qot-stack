# Content description #

All source files in the root directory are compiled together to form the **qot_core** kernel module, which is the heart of the QoT stack. This code is divided into the following key modules:

1. **qot_admin*.{c,h}** - administrative interface to the QoT core - getting and setting OS latency, timeline management, etc. initialization of the chdev interface /dev/qotadm (qot_admin_chdev.c) and sysfs introspection at /dev/class/qot/qotadm (qot_admin_sysfs.c).
2. **qot_clock.{c,h}** - Supports the registration and unregistration of platform clocks. Internally, maintains a red-black tree. 
3. **qot_scheduler.{c,h}** - The QoT scheduler code (WIP).
4. **qot_timeline*.{c,h}** - Supports the creation, destruction of timelines, stored internally as a red-black tree. The qot_timeline_chdev.c creates character device /dev/timelineX that provides a POSIX_CLOCK representing the timeline and supports ioctl meta-calls.  The file qot_timeline_sysfs.c provides timeline introspection at /sys/class/qot/timelineX.
5.  **qot_user*.{c,h}** - user interface to the QoT core - listing and creating new timelines through chdev interface /dev/qotusr (qot_user_chdev.c).
3. **qot_core.h** - Interface used by platform clocks to interact with core.
4. **qot_core.c** - The base "main" function of the kernel module.
