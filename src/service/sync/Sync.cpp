/**
 * @file Sync.cpp
 * @brief Factory class prepares a synchronization session
 * @author Fatima Anwar
 * 
 * Copyright (c) Regents of the University of California, 2015. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *      1. Redistributions of source code must retain the above copyright notice, 
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
 *
 *
 */

#include "Sync.hpp"

// Subtypes
#include "ptp/PTP.hpp"
#include "ntp/NTP.hpp"

#define QOT_IOCTL_BASE		"/dev"
#define QOT_IOCTL_PTP		"ptp"
#define QOT_IOCTL_FORMAT	"%3s%d"
#define QOT_MAX_NAMELEN 	(32)

using namespace qot;

// Factory method
boost::shared_ptr<Sync> Sync::Factory(
			boost::asio::io_service *io,		// IO service
			const std::string &address,			// IP address of the node
			const std::string &iface)			// interface for synchronization
{

	// If IP address is private (LAN) and interface specified is ethernet
	if(IsIPprivate(address) && strncmp(iface.c_str(),"eth",3)==0){

		// Open dev directory
		DIR *dir = opendir(QOT_IOCTL_BASE);
		if(dir){
			BOOST_LOG_TRIVIAL(info) << "directory open" << QOT_IOCTL_BASE;
			// Read the entries from dev directory
    		struct dirent *entry;
			while ((entry = readdir(dir)) != NULL) {
				BOOST_LOG_TRIVIAL(info) << "entry in directory found" << QOT_IOCTL_BASE;
				// Check if we have a ptp device
				char str[QOT_MAX_NAMELEN]; 
				int ret, val;	
				ret = sscanf(entry->d_name, QOT_IOCTL_FORMAT, str, &val);
				if ((ret==2) && (strncmp(str,QOT_IOCTL_PTP,QOT_MAX_NAMELEN)==0)){
					BOOST_LOG_TRIVIAL(info) << "found in directory ptp" << val;
					closedir(dir);
    				return boost::shared_ptr<Sync>((Sync*) new PTP(io,iface,val)); 	// Instantiate a ptp sync algorithm
    			}
    		}
    		//BOOST_LOG_TRIVIAL(error) << "No entry in the directory " << QOT_IOCTL_BASE;
    	}else{
    		BOOST_LOG_TRIVIAL(error) << "Could not open the direcotry " << QOT_IOCTL_BASE;
    	}
    }
    return boost::shared_ptr<Sync>((Sync*) new NTP(io,iface)); 				// Instantiate ntp sync algorithm
}

uint32_t Sync::IPtoUint(const std::string ip) {
    int a, b, c, d;
    uint32_t addr = 0;

    if (sscanf(ip.c_str(), "%d.%d.%d.%d", &a, &b, &c, &d) != 4)
        return 0;

    addr = a << 24;
    addr |= b << 16;
    addr |= c << 8;
    addr |= d;
    return addr;
}

bool Sync::IsIPprivate(const std::string ip) {
    uint32_t ip_addr = IPtoUint(ip);

    uint32_t classA_mask = 0x0A000000; // 10.0.0.0 ~ 10.255.255.255
    uint32_t classB_mask = 0xAC100000; // 172.16.0.0 ~ 172.31.255.255
    uint32_t classC_mask = 0xC0A80000; // 192.168.0.0 ~ 192.168.255.255

    if(((ip_addr & classA_mask) == classA_mask) || 
    	((ip_addr & classB_mask) == classB_mask) || 
    	((ip_addr & classC_mask) == classC_mask)){

    	return true;
    }
    return false;
}
