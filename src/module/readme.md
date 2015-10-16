Here is a brief description of the module hierarchy

- qot : responsible for the construction and destruction of timelines
	- qot_am335x : init code for BeagleBone Black architecture that calls qot hooks
		- qot_am335x_chronos : injects Chronos board oscillator 
		- qot_am335x_gpsdo : adds a GPSDO
		- qot_am335x_lowpower : adds the lowpower oscillator to the 