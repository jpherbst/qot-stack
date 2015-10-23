/*
 * @file helloworld_timeline.cpp
 * @brief Simple C++ example showing how to register to listen for capture events
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
#define WAIT_TIME_SECS   1
#define NUM_ITERATIONS   10
#define OFFSET_MSEC      1000

// This function is called by the QoT API when a capture event occus
void callback(const char *pname, int64_t epoch)
{
	std::cout << "Event captured at time " << epoch << std::endl;
}

// Main entry point of application
int main(int argc, char *argv[])
{
	// Bind to a timeline
	try
	{
		qot::Timeline timeline(TIMELINE_UUID, 1e6, 1e3);	
		for (int i = 0; i < NUM_ITERATIONS; i++)
		{
			int64_t tval = timeline.GetTime();
			std::cout << "[Iteration " << (i+1) << "] " << tval << std::endl;
			std::cout << "WAITING FOR " << OFFSET_MSEC << "ms AT " << tval << std::endl;
			timeline.WaitUntil(tval + OFFSET_MSEC * 1e6);
			std::cout << "RESUMED AT " << timeline.GetTime() << std::endl;
		}
	}
	catch (std::exception &e)
	{
		std::cout << "EXCEPTION: " << e.what() << std::endl;
	}

	// Unbind from timeline
	return 0;
}