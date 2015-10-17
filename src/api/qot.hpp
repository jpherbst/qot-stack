/*
 * @file qot.hpp
 * @brief Userspace C++ API to manage QoT timelines
 * @author Fatima Anwar 
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

#ifndef QOT_HPP
#define QOT_HPP

#include <exception>

extern "C"
{
	#include <unistd.h>
	#include <fcntl.h>
	#include <stdio.h>
	#include <string.h>
	#include <stdint.h>
	#include <time.h>
}

namespace qot
{
	/**
	 * @brief Various exceptions
	 */
	struct CannotCommunicateWithCoreException : public std::exception {
		const char * what () const throw () {
			return "Cannot bind to a timeline";
		}
	};
	struct ProblematicUUIDException : public std::exception {
		const char * what () const throw () {
			return "The specified UUID exceeds the size limit";
		}
	};
	struct CannotBindToTimelineException : public std::exception {
		const char * what () const throw () {
			return "Cannot bind to a timeline";
		}
	};
	struct CannotOpenPOSIXClockException : public std::exception {
		const char * what () const throw () {
			return "Cannot open the POSIX clock";
		}
	};
	struct CommunicationWithCoreFailedException : public std::exception {
		const char * what () const throw () {
			return "The qot core respodned with an error";
		}
	};

	/**
	 * @brief Bind to a timeline
	 */
	class Timeline
	{
		/**
		 * @brief Bind to a timeline
		 * @param uuid A unique identifier for the timeline
		 * @param acc Required accuracy
		 * @param res Required resolution
		 * @return Positive: success Negative: error
		 **/
		public: Timeline(const char* uuid, uint64_t acc, uint64_t res);

		/**
		 * @brief Unbind from a timeline
		 **/
		public: ~Timeline();

		/**
		 * @brief Update the requested binding accuracy
		 * @param bid The binding id
		 * @param accuracy The new accuracy
		 * @return Positive: success Negative: error
		 **/
		public: int SetAccuracy(uint64_t acc);

		/**
		 * @brief Update the requested binding resolution
		 * @param bid The binding id
		 * @param accuracy The new resolution
		 * @return Positive: success Negative: error
		 **/
		public: int SetResolution(uint64_t res);

		/**
		 * @brief Get the current global timeline
		 * @return The time
		 **/
		public: int64_t GetTime();

		/**
		 * @brief Request a capture action on a given pin
		 * @param bid The binding id
		 * @param pname The pin name (must be offered by the driver)
		 * @param enable Turn compare on or off 
		 * @param callback Callback funtion for capture
		 * @return 0+: success, <0: error
		 **/
		public: int RequestCapture(const char* pname, uint8_t enable,
			void (callback)(const char *pname, int64_t val));

		/**
		 * @brief Request a compare action on a given pin
		 * @param bid The binding id
		 * @param pname The pin name (must be offered by the driver)
		 * @param enable Turn compare on or off 
		 * @param start The global start time
		 * @param high The high time of the duty cycle
		 * @param low The low time of the duty cycle
		 * @param limit The total number of cycles (0: infinite)
		 * @return 0+: success, <0: error
		 **/
		public: int RequestCompare(const char* pname, uint8_t enable, 
			int64_t start, uint64_t high, uint64_t low, uint64_t limit);

		/**
		 * @brief Request a compare action on a given pin
		 * @param bid The binding id
		 * @param val The global time to wakeup
		 * @return 0+: success, <0: error
		 **/
		public: int WaitUntil(uint64_t val);

		/**
		 * @brief Internal data structures
		 **/
		private: int fd_qot;	// Descriptor for qot ioctl
		private: int fd_clk;	// Descriptor for timeline
		private: int bid;		// Binding ID
		private: clockid_t clk;	// POSIX clock ID
	};
}

#endif
