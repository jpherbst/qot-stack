## Ideal HOWTO: syncing two oscillators with each other
```
$ /usr/bin/phc2phc --algo "linear" -m /dev/phc4 -s /dev/phc5 -p 1000msec
```
```
master$ # start ptp broadcast of timeline 7 on network 
master$ ptp4l -m -i /dev/timeline7
```

```
slave$ # start ptp listening for timeline 7 on network
slave$ ptp4l -s -i /dev/timeline7
```

## Essential Modules
```
$ lsmod		# what modules do we have on a fully running system
	qot_core	# essential for QoT management
	qot_am335x	# platform clock provide interrupts
	qot_chronos # platform clock provide interrupts
	dw1000		# qot_agnostic (decawave 1000): regular NIC
	cc1200		# qot_agnostic (CC 900MHz radios): regular NIC
	cpsw		# qot_agnostic (common platform switch aka ethernet): regular NIC
```

## Requirements for platform oscillator

1. see primary timer (`read_time()` - core timing)
2. set secondary timer (`setI_iterrupts(x)` - interval timing)
3. expose clock+pins as PHC (show as `/dev/ptpX`)
4. register with qot_core (call `qot_core_register()` at load & ``)


```

struct time_range {
	ktime_t min;
	ktime_t max;
};

enum clk_err {
	ppb = 0,
	NCLKERR
};

struct platform_clock_property {
	// nominal characteristics
	s64 nominal_freq;					// or a representation of
	s64 nominal_power;			// representation of power consumption

	// communication uncertainities
	struct time_range read_latency;
	struct time_range interrupt_latency;
	
	// oscillator uncertainities
	s64 errors[NCLKERR]; // strongly optional
};

// info to send with qot_platform_clock_register(...)

struct qot_platform_clock_info {
	char name[MAX_NAME];
	struct platform_clock_property properties;
	struct ptp_clock_info ptpclk;		// PTP device representing oscillator

	ktime_t (*read_time)(void);			// get clock time
	long (*program_interrupt)(ktime_t expiry, long (*callback)(void*));	long (*cancel_interrupt)(void);		// cancel the last programmed interrupt
};

struct qot_platform_clock {	
	struct qot_platform_clock_info;
	ktime_t last_accuracy_update;
	// ... additional private stuff required
};
```



## Requirements for network oscillator

1. Must support packet timestamping
2. see primary timer (`read_time()` - core timing)
3. Suggested support: expose clock as PHC (show as `/dev/ptpX`), give SFD. Failsafe abstraction for everything else

## core clock data structure

```
qot_platform_clock_register(struct qot_platform_clock_info *info);
qot_platform_clock_unregister(struct qot_platform_clock_info *info);
qot_platform_clock_property_update(struct qot_platform_clock_info *info);
```

## qot_core task list
1. `sysfs` introspection/management on qot_stack (eg. `/dev/qot` etc and other files for osc and core and timelines)
2. parallel high-resolution timer system running on platform clock (Sandeep)
3. veriy what way ioctl uses files in? (does it serialize? can multiple apps access at the same time). NOTE: /dev/qot requires parallel access
4. `/dev/qot_admin` for superuser only priviledged access (eg. switching clocks). eg. sync & optimization code in user space is responsible for managing these (Andrew)
5. Simpler clocksource parallel implementation. core <---> timer interface (Sandeep)
6. AM335x module should create new timer device while bringing up new timer for interrupts (Sandeep)
7. Uncertainity propogation up the stack with each call (Adwait)
8. Sync algorithm and daemon in userspace (Fatima & Zhao. Andrew for phc2phc)
9. Time projection operations - core <---> timelines interface (Andrew & Adwait)

## `/dev/qot_admin` features

Management interface for qot
Supports:

```
sync_set_uncertainity(timeline)
get_available_clocks(void)
set_clock(platform_clock_name)
get_clock(void)
set_os_latency(interval_t)
get_os_latency(void)
```

## DDS and OpenSplice implementation

- Easier configurations
- uses ospl.conf for configuration
- We use PrismTech OpenSplice LGPL
- openDDS is not friendly