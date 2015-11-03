/*
 * @file qot.hpp
 * @brief Userspace C++ API to manage QoT timelines
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

#ifndef QOT_HPP
#define QOT_HPP

#include <string>
#include <exception>
#include <thread>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <map>
#include <algorithm>

namespace qot
{
	/**
	 * @brief Timeline events
	 */
	typedef enum {
		EVENT_BIND,
		EVENT_UNBIND
	} TimelineEventType;

	/**
	 * @brief Convenience decalaration
	 */
	typedef std::function<void(const std::string &pname, int64_t val)> CaptureCallbackType;
	typedef std::function<void(const std::string &name, int8_t event)> EventCallbackType;

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
	struct ProblematicPinNameException : public std::exception {
		const char * what () const throw () {
			return "The specified pin name exceeds the size limit";
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
	struct CommunicationWithPOSIXClockException : public std::exception {
		const char * what () const throw () {
			return "The qot core respodned with an error";
		}
	};

	/**
	 * @brief The timeline class is the primary interface between user-space code and
	 *        the quality of time stack. 
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
		public: Timeline(const std::string &uuid, uint64_t acc, uint64_t res);

		/**
		 * @brief Unbind from a timeline
		 **/
		public: ~Timeline();

		/**
		 * @brief Set the desired binding accuracy
		 * @param accuracy The new accuracy
		 * @return Positive: success Negative: error
		 **/
		public: int SetAccuracy(uint64_t acc);

		/**
		 * @brief Set the desired binding resolution
		 * @param res The new resolution
		 * @return Positive: success Negative: error
		 **/
		public: int SetResolution(uint64_t res);

		/**
		 * @brief Get the current global timeline
		 * @return The time (in nanoseconds) since the Unix epoch
		 **/
		public: int64_t GetTime();

		/**
		 * @brief Get the accuracy achieved by the system
		 * @return The current accuracy
		 **/
		public: uint64_t GetAccuracy();

		/**
		 * @brief Get the resolution achieved by the system
		 * @return The current accuracy
		 **/
		public: uint64_t GetResolution();

		/**
		 * @brief Get the name of this application (how it presents itself to peers)
		 * @return The name of the binding / application
		 **/
		public: std::string GetName();

		/**
		 * @brief Get the name of this application (how it presents itself to peers)
		 * @param name the new name for this application
		 **/
		public: void SetName(const std::string &name);

		/**
		 * @brief Request a compare action on a given pin
		 * @param pname The pin name (must be offered by the driver)
		 * @param enable Turn compare on or off 
		 * @param start The global start time
		 * @param high The high time of the duty cycle
		 * @param low The low time of the duty cycle
		 * @param limit The total number of cycles (0: infinite)
		 * @return 0+: success, <0: error
		 **/
		public: int GenerateInterrupt(const std::string &pname, uint8_t enable, 
			int64_t start, uint32_t high, uint32_t low, uint32_t repeat);

		/**
		 * @brief Request to be notified of network events (device, action)
		 * @param callback Callback funtion for device binding
		 **/
		public: void SetEventCallback(EventCallbackType callback);

		/**
		 * @brief Request to be notified when a capture event occurs
		 * @param callback Callback funtion for capture
		 **/
		public: void SetCaptureCallback(CaptureCallbackType callback);

		/**
		 * @brief Wait until some global time (if in past calls back immediately)
		 * @param val Global time
		 * @return Predicted error 
		 **/
		public: int64_t WaitUntil(int64_t val);

		/**
		 * @brief Sleep for a given number of nanoseconds relative to the call time
		 * @param val Number of nanoseconds
		 * @return Predicted error 
		 **/
		public: int64_t Sleep(uint64_t val);

		/**
		 * @brief Blocking poll on fd for capture events
		 **/
		private: void CaptureThread();

		/**
		 * @brief Create a random string of fixed length
		 * @param length the length of the string
		 * @return the random string
		 **/
		private: static std::string RandomString(uint32_t length);

		/**
		 * @brief Internal data structures
		 **/
		private: std::string name;					// Unique ID for this application
		private: bool kill;							// Kill flag for thread
		private: int fd_qot;						// Descriptor for qot ioctl
		private: int fd_clk;						// Descriptor for timeline
		private: clockid_t clk;						// POSIX clock ID
		private: std::thread thread;				// Worker thread for capture
		private: CaptureCallbackType cb_capture;	// Callback for captures
		private: EventCallbackType cb_event;		// Callback for devices
		private: std::mutex m;						// Mutex
		private: std::condition_variable cv;		// Conditional variable
		private: std::unique_lock<std::mutex> lk;	// Lock
	};
}

#endif
