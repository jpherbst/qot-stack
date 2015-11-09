/**
 * @file Coordination.hpp
 * @brief Library to coordinate the election and maintenance of a master clock
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
#ifndef COORDINATION_HPP
#define COORDINATION_HPP

/* Boost includes */
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

/* IDL messages */
#include "msg/QoT_DCPS.hpp"
#include "msg/TempControl_DCPS.hpp"

/* Convenience */
std::ostream& operator<< (std::ostream& os, const tutorial::TempSensorType& ts);

namespace qot
{
	////////////////////////////////////////////////////////////////////////////////////////////

	class TempSensorListener : public dds::sub::NoOpDataReaderListener<tutorial::TempSensorType> {
	public:
	virtual void 
	on_data_available(dds::sub::DataReader<tutorial::TempSensorType>& dr) 
	{ 
		std::cout << "----------on_data_available-----------" << std::endl;      
		auto samples =  dr.read();
		std::for_each(samples.begin(), samples.end(), [](const dds::sub::Sample<tutorial::TempSensorType>& s) {
			std::cout << s.data() << std::endl;
		});
	}

	virtual void 
	on_liveliness_changed(dds::sub::DataReader<tutorial::TempSensorType>& dr, 
		const dds::core::status::LivelinessChangedStatus& status) 
		{
			std::cout << "!!! Liveliness Changed !!!" << std::endl;
		}
	};

	////////////////////////////////////////////////////////////////////////////////////////////
	
	/* Performs coordination function across multiple entities in the network */
	class Coordination
	{

	// Constructor and destructor
	public: Coordination(boost::asio::io_service *io, const std::string &dir);
	public: ~Coordination();
	
	// ASIO variables
	private: void Heartbeat(const boost::system::error_code& /*e*/);

	// Private variables
	private: boost::asio::io_service *asio;
	private: std::shared_ptr<boost::asio::deadline_timer> timer;

	// DDS variables
	private: tutorial::TempSensorType timeline;
	private: dds::domain::DomainParticipant dp;
	private: dds::topic::Topic<tutorial::TempSensorType> topic;
	private: dds::pub::Publisher pub;
  	private: dds::sub::Subscriber sub;
	private: dds::pub::DataWriter<tutorial::TempSensorType> dw;
  	private: dds::sub::DataReader<tutorial::TempSensorType> dr;
  	private: TempSensorListener listener;

	};
}

#endif
