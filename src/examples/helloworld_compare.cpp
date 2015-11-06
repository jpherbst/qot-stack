/*
 * @file helloworld_compare.cpp
 * @brief Simple C++ example showing how to generate a deteministic external event
 * @author Andrew Symington and Fatima Anwar 
 * 
 * Copyright (c) Regents of the University of California, 2015. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 * 	1. Redistributions of source code must retain the above copyright notice, 
 *     this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice, 
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// C++ includes
#include <iostream>

// Include the QoT API
#include "../api/qot.hpp"

// Basic onfiguration
#define TIMELINE_UUID    "my_test_timeline"
#define CAPTURE_PIN_UUID "timer7"
#define OFFSET_NSEC      10e9
#define START_NSEC   	 1e9
#define HIGH_NSEC        1e6
#define LOW_NSEC   		 1e6
#define CYCLE_LIMIT   	 5

// Main entry point of application
int main(int argc, char *argv[])
{
	// Bind to a timeline
	try
	{
		// Bind to the timeline
		qot::Timeline timeline(TIMELINE_UUID, 1e6, 1e3);
		timeline.GenerateInterrupt(CAPTURE_PIN_UUID, 1, timeline.GetTime() + START_NSEC, HIGH_NSEC, LOW_NSEC, CYCLE_LIMIT);
		std::cout << "WAITING FOR " << OFFSET_NSEC << "ns FOR COMPARE" << std::endl;
		timeline.Sleep(OFFSET_NSEC);
	}
	catch (std::exception &e)
	{
		std::cout << "EXCEPTION: " << e.what() << std::endl;
	}

	// Unbind from timeline
	return 0;
}
