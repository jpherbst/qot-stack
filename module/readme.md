/////////////////////////////////// OVERVIEW //////////////////////////////////

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

* qot.ko
  * qot_core     : provides ioctl (/dev/qot) to listen for user API requests
  * qot_timeline : creates timelines and bindings representing shared time
  * qot_clock    : creates a POSIX clock (/dev/timelineX) for each timeline
* qot_am335x.ko
  * qot_am335x   : sets up timer pins for the BeagleBone Black

////////////////////////////// CAPTURE EVENTS //////////////////////////////////

KERNEL PERSPECTIVE: What happens in a capture event

- The QoT platform driver (qot_am335x) parses the device tree to setup capture
  timers on a subset of pins. Each pin is registered with qot_core
- The hardware shadow copies the current dmtimer count into a capture register
- A kernel IRQ is then called with a capture irqflag set
- The corresponding ISR collects the dmtimer capture register and uses the
  cycle counter to determine the int64_t reperesntation of time
- The ISR then schedules delayed work with the (timer_id, int64_t) as the data
- At some later point the top half of the kernel processes the delayed work,
  which means that it passes the data to QoT core throuh an exported function
- The QoT core then inserts the capture request into every queue offered by
  bindings that have registered to hear such events. The file desriptor 
  attached to each binding is then polled with POLLRD to signal new capture data

  timer4
    - bid1 => C7
    - bid4 => C4 C5 C6 C7
    - bid7 => C6 C7
  timer5
    - bid2 => C1 C2 C3
    - bid3 => C2 C3
    - bid9 => C1 C2 C3

USER PERSPECTIVE: How capture data is obtained

- The user creates a timer object that binds it to a timeline
- The user creates a callback of type int(string a, int64_t) to listen
- The user requests an interrupt

////////////////////////////// COMAPRE EVENTS //////////////////////////////////