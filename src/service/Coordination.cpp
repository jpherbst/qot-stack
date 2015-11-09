/*
 * @file Coordination.cpp
 * @brief Library to coordinate 
 * @author Andrew Symington
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

/* This file header */
#include "Coordination.hpp"

/* Trivial logging */
#include <boost/log/trivial.hpp>

/* PAarameters */
#define HEARTBEAT_MS 1000

using namespace qot;

////////////////////////////////////////////////////////////////////////////////////////////

std::ostream& operator <<(std::ostream& os, const tutorial::TempSensorType& ts)
{
  os << "(id = " << ts.id()  
     << ", temp = " << ts.temp()
     << ", hum = " << ts.hum()
     << ", scale = " << ts.scale()
     << ")";
  return os;
}

////////////////////////////////////////////////////////////////////////////////////////////

Coordination::Coordination(boost::asio::io_service *io, const std::string &dir)
	: asio(io), timer(std::make_shared<boost::asio::deadline_timer>(*io, boost::posix_time::milliseconds(HEARTBEAT_MS))),
		dp(0), timeline(18, 0.1, 0.4, tutorial::CELSIUS), topic(dp, "TTempSensor"), pub(dp), sub(dp), dr(sub, topic), dw(pub, topic)
{
	/* Setup a data listener to be called with new data */
  	dr.listener(boost::bind(&Coordination::listener, &this->listener), dds::core::status::StatusMask::data_available());
  	
	/* Setup a hearbeat timer */
	timer->async_wait(boost::bind(&Coordination::Heartbeat, this, boost::asio::placeholders::error));
}

Coordination::~Coordination()
{
	BOOST_LOG_TRIVIAL(info) << "KILLED";

	/* Destroy the listener */
	dr.listener(nullptr, dds::core::status::StatusMask::none());
}

void Coordination::Heartbeat(const boost::system::error_code& /*e*/)
{
	BOOST_LOG_TRIVIAL(info) << "Heartbeat";
	
	/* Publish info about the timeline */
	// dw.write(timeline);

	/* Reset timer */
   	timer->expires_at(timer->expires_at() + boost::posix_time::milliseconds(HEARTBEAT_MS));
    timer->async_wait(boost::bind(&Coordination::Heartbeat, this, boost::asio::placeholders::error));
}