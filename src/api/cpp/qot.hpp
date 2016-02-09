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
​
#ifndef QOT_STACK_SRC_API_CPP_QOT_H
#define QOT_STACK_SRC_API_CPP_QOT_H
​
#include "qot_types.h"

namespace qot
{
	/**
	 * @brief Convenience decalaration
	 */
	typedef std::function<void(TimelineEventType event,
		const std::string &data, timeest_t eventtime)> EventCallbackType;
​
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
​
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
​
//////////////
		 //////////////
		 //////////////
		 ////////////// Use #define msec, usec, nsec in header files and use in calls
		 ////////////// basic datatypes: time_t (signed), interval_t ({+ve, +ve}), duration_t ({+ve}), durationest_t ({duration_t, interval_t})
		 //////////////
​
		public:
			Timeline(const std::string &uuid, duration_t resolution, interval_t accuracy);

		//	accuracy_lower_limit: t_true >= t_returned - accuracy_lower_limit
		//	accuracy_upper_limit: t_true <= t_returned + accuracy_upper_limit
​
		/**
		 * @brief Unbind from a timeline
		 **/
		public: ~Timeline();
​
		/**
		 * @brief Set the desired binding accuracy
		 * @param accuracy The new accuracy
		 * @return Positive: success Negative: error
		 **/
		public:
			int SetAccuracy(interval_t accuracy);

		/**
		 * @brief Set the desired binding resolution
		 * @param res The new resolution
		 * @return Positive: success Negative: error
		 **/
		public: int SetResolution(duration_t res);
​
		/**
		 * @brief Get the current global timeline
		 * @return The time (in nanoseconds) since the Unix epoch
		 **/
		public: timeest_t GetTime();
​
		/**
		 * @brief Get the accuracy achieved by the system
		 * @return The current accuracy
		 **/
		public: interval_t GetAccuracy();
​
		/**
		 * @brief Get the resolution achieved by the system
		 * @return The current accuracy
		 **/
		public: duration_t GetResolution();
​
		/**
		 * @brief Get the name of this application (how it presents itself to peers)
		 * @return The name of the binding / application
		 **/
		public: std::string GetName();
​
		/**
		 * @brief Get the name of this application (how it presents itself to peers)
		 * @param name the new name for this application
		 **/
		public: void SetName(const std::string &name);
​
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
		public:
			int ConfigPinInterrupt(const std::string &pname, time_t start, interval_t period);
			int CancelPinInterrupt(const std::string &pname);
​
			int ConfigPinTimestamp(const std::string &pname, transition_t edge, EventCallbackType TimestampCallback);
			int CancelPinTimestamp(const std::string &pname);
​
		/**
		 * @brief Request to be notified of network events (device, action)
		 * @param callback Callback funtion for device binding
		 **/
		public: int TimelineEventCallback(TimelineEventType type, EventCallbackType callback);
		public: int CancelTimelineEventCallback(TimelineEventType type);
​
​
​
		/**
		 * @brief Wait until some global time (if in past calls back immediately)
		 * @param val Global time
		 * @return Predicted error
		 **/
		public: timeest_t WaitUntil(timeest_t absolute_time);
​
		public: int CreateTimer(
			const std::string &name,
			timeest_t start_time, durationest_t period, EventCallbackType TimerCallback, int count);
		public: int DestroyTimer(const std::string &name);
​
​
		// period is
​
​
		/**
		 * @brief Sleep for a given number of nanoseconds
		 * @param val Number of nanoseconds
		 * @return Predicted error
		 **/
		public: timeest_t Sleep(durationest_t delta);
	};
}
​
#endif