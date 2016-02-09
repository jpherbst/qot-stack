/*
 * struct qot_driver - Callbacks that allow the QoT to core hooks into the hardware
 *                     timers. Implementation is really up to the driver.
 *
 * @compare:            read the current "core" time value in nanoseconds
 * @compare:            action a pin interrupt for the given period
 */

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

struct qot_platform_clock_info {
    char name[MAX_NAME];
    struct platform_clock_property properties;
    struct ptp_clock_info ptpclk;       // PTP device representing oscillator

    ktime_t (*read_time)(void);         // get clock time
    long (*program_interrupt)(ktime_t expiry, long (*callback)(void*)); long (*cancel_interrupt)(void);     // cancel the last programmed interrupt
};

// Register a given clocksource as the primary driver for time
long qot_platform_clock_register(struct qot_platform_clock_info *info);


// Unregister the  clock source, oscillators and pins
long qot_platform_clock_unregister(struct qot_platform_clock_info *info);

long qot_platform_clock_property_update(struct qot_platform_clock_info *info);
