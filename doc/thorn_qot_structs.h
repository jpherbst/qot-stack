struct freq_struct {
	long mult;
	long shift;
};

struct accuracy_struct {
	// communication uncertainities
	long latency_min, latency_max;
	struct timespec max_tick_resolution;
	long min_tick_freq; // this is the same as 1/max_tick_resolution

	// oscillator uncertainities
	long ppb;
	long allan_dev[NDEV];
};


// timeline level
struct qot_timeline {
	struct ptp_dev ptp;		// also creates a chardev /dev/ptpX
	char name[NSIZE];
};


struct core_clock { // only be one of these
	long mult;
	long shift;
	long offset;
	struct oscillator *osc;	// pointer to current osc datastruct
	long (*setAccuracy)(struct accuracy_struct acc);
	long (*update_osc_params)(...);
};

struct network_clock { // only be one of these
	long mult;
	long shift;
	long offset;
	struct oscillator *osc;	// pointer to current osc datastruct
	long (*setAccuracy)(struct accuracy_struct acc);
};

struct core_time sys_core;
EXPORT_SYMBOL(sys_core);

// oscillator level
struct oscillator {
	struct freq_struct freq;	// or a representation of this
	struct accuracy_struct ppb;	// some representation of accuracy: latency, jitter
	struct power_struct power;	// representation of power consumption
	long (*setAccuracy)(struct accuracy_struct acc);
	struct ptp_clock_info ptpclk;				// PTP device representing osc
};

struct timespec *gettime(struct qot_timeline *tl) {
	return gettime(core_time_device);	
}

struct timespec *gettime(struct core_time *ct) {
	return count2time(ct->osc->count, freq_struct);	
}

// pin change function call
// method 1: delegate
trigger_pin_hardware() {
	set_pin(44, HIGH, 13371337);	// blocking call
}

// method 2: OS is responsible for timing
trigger_pin_software() {
	wait_until(13371337);
	set_pin(44, HIGH);
}

// synchronization class algoithm
long generic_sync_algo(char *strategy, struct qot_timeline *tl1, struct qot_timeline *tl2, ...) {

}