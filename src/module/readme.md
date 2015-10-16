The qot.ko module is the core Quality of Time module, and is responsible for the
creation and destruction of timelines, and their corresponding POSIX clocks. To
summarize, a hash map is used to store clocks by their character UUID. A user-
space application requests a binding to a timeline through a user-space API that
maps the command into an ioctl call to the kernel module. The IDR kernel library
is used to dish out a descriptor-like integer index that uniquely references the
binding. If a request is made to access a timeline that has not yet been seen by
the qot module, then a new timeline is created along with a POSIX clock. This 
clock is exposed to user-space at /dev/timelineX. The idea is that a user-space
synchronization algorithm then disciplines this clock using PTP or NTP. Most 
importantly, this clock also offers ioctl access to query the target 
