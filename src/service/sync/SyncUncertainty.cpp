/**
 * @file SyncUncertainty.cpp
 * @brief Synchronization Uncertainty Estimation Logic
 *        based on "Safe Estimation of Time Uncertainty of Local Clocks ISPCS 2009"
 * @author Sandeep D'souza
 *
 * Copyright (c) Carnegie Mellon University, 2017. All rights reserved.
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

#include "SyncUncertainty.hpp"

#include "ProbabilityLib.hpp"

/* So that we might expose a meaningful name through PTP interface */
#define QOT_IOCTL_BASE          "/dev"
#define QOT_IOCTL_PTP           "ptp"
#define QOT_IOCTL_PTP_FORMAT    "%3s%d"
#define QOT_MAX_PTP_NAMELEN     32

using namespace qot;

// Constructor 
SyncUncertainty::SyncUncertainty(struct uncertainty_params uncertainty_config)
: drift_popvar(0), drift_samvar(0), offset_popvar(0), drift_bound(0), 
  offset_bound(0), drift_pointer(0), offset_pointer(0), config{50,50,0.999999,0.999999,0.999999,0.999999}
{
	// Configure the parameters
	Configure(uncertainty_config);
}

// Constructor 2
SyncUncertainty::SyncUncertainty()
: drift_popvar(0), drift_samvar(0), offset_popvar(0), drift_bound(0), 
  offset_bound(0), drift_pointer(0), offset_pointer(0), config{50,50,0.999999,0.999999,0.999999,0.999999}
{
	return;
}

// Destructor
SyncUncertainty::~SyncUncertainty() {}

// Add Latest Statistic and Calculate Bounds
bool SyncUncertainty::CalculateBounds(int64_t offset, double drift, int timelinefd)
{
	qot_bounds_t bounds; // Calculated bound values

	// Add Newest Sample
	AddSample(offset, drift);

	if(drift_samples.size() < config.M && offset_samples.size() < config.N)
	{
		// Insufficient samples for calculating uncertainty
		return false;
	}

	// Calculate Drift and Offset Variance Bounds
	CalcVarBounds();

	// Predictor Function Coefficients (Needs to be multiplied with (t-t0)^(3/2))
	//right_predictor = 2*inv_error_pdv*sqrt(diffusion_coef)/3; // Like the paper we assume that the sync algorithm corrects the offset and drift
	right_predictor = 2*upper_confidence_limit_gaussian(sqrt(drift_bound),config.pdv)/3;
	left_predictor = -right_predictor;

	// Margin Functions
	right_margin = sqrt(2)*inv_error_pov*sqrt(offset_bound);
	left_margin = -right_margin;

	std::cout << "Right Predictor = " << right_predictor
	          << " Right Margin = " << right_margin << "\n";

	// Poulate the bounds
	bounds.u_drift = (s64)ceil(right_predictor*1000000000LL); // Upper bound (Right Predictor) function for drift
	bounds.l_drift = (s64)ceil(left_predictor*1000000000LL);  // Lower bound (Left Predictor) function for drift
	bounds.u_nsec  = (s64)ceil(right_margin);                 // Upper bound (Right Margin) function for offset
	bounds.l_nsec  = (s64)ceil(left_margin);                  // Lower bound (Left Margin) function for offset

	// Write calculated uncertainty information to the timeline character device
	if(ioctl(timelinefd, TIMELINE_SET_SYNC_UNCERTAINTY, &bounds)){
		std::cout << "Setting sync uncertainty failed for timeline\n";
	}

	return true;
}

// Configure the Parameters of the Synchronization Uncertainty Calculation Algorithm
void SyncUncertainty::Configure(struct uncertainty_params configuration)
{
	config = configuration;
	inv_error_pov = get_inverse_error_func(config.pov);
	return;
}

// Add a new sample to the vector queues
void SyncUncertainty::AddSample(int64_t offset, double drift)
{
	// Add new drift sample
	if (drift_samples.size() < config.M)
	{
		drift_samples.push_back(drift);
	}
	else
	{
		drift_samples[drift_pointer] = drift;
	}
	drift_pointer = (drift_pointer + 1) % config.M;

	// Add new offset sample
	if (offset_samples.size() < config.N)
	{
		offset_samples.push_back(offset);
	}
	else
	{
		offset_samples[offset_pointer] = offset;
	}
	offset_pointer = (offset_pointer + 1) % config.N;

	return;
}

// Calculate population variance
double SyncUncertainty::GetPopulationVarianceDouble(std::vector<double> samples)
{
	double var = 0;
	double mean = 0;
	int numPoints = samples.size();
	int n;

	// Calculate Mean
	for(n = 0; n < numPoints; n++)
	{
		mean = mean + samples[n];
	}
	mean = mean/numPoints;

	// Calculate Variance
	for(n = 0; n < numPoints; n++)
	{
	   var += (samples[n] - mean)*(samples[n] - mean);
	}
	var = var/numPoints;

	return var;
}

// Calculate population variance
double SyncUncertainty::GetPopulationVariance(std::vector<int64_t> samples)
{
	double var = 0;
	double mean = 0;
	int numPoints = samples.size();
	int n;

	// Calculate Mean
	for(n = 0; n < numPoints; n++)
	{
		mean = mean + (double)samples[n];
	}
	mean = mean/numPoints;

	// Calculate Variance
	for(n = 0; n < numPoints; n++)
	{
	   var += ((double)samples[n] - mean)*((double)samples[n] - mean);
	}
	var = var/numPoints;

	return var;
}

// Calculate sample variance
double SyncUncertainty::GetSampleVarianceDouble(std::vector<double> samples)
{
	double var = 0;
	double mean = 0;
	int numPoints = samples.size();
	int n;

	// Calculate Mean
	for(n = 0; n < numPoints; n++)
	{
		mean = mean + samples[n];
	}
	mean = mean/numPoints;

	// Calculate Variance
	for(n = 0; n < numPoints; n++)
	{
	   var += (samples[n] - mean)*(samples[n] - mean);
	}
	var = var/(numPoints-1);

	return var;
}

// Calculate variance bounds
void SyncUncertainty::CalcVarBounds()
{
	// Calculate Variances
	drift_popvar  = GetPopulationVarianceDouble(drift_samples);   // Drift population variance
    drift_samvar  = GetSampleVarianceDouble(drift_samples);       // Drift Sample Variance
    offset_popvar = GetPopulationVariance(offset_samples);  // Offset Population Variance

    std::cout << "Drift Variance = " << drift_popvar << "\n";
    // Get the upper bound on the drift variance using the chi squared distribution
    drift_bound = upper_confidence_limit_on_std_deviation(sqrt(drift_popvar), config.M, config.pds);

    std::cout << "Offset Variance = " << offset_popvar << "\n";
    // Get the upper bound on the offset variance using the chi squared distribution
    offset_bound = upper_confidence_limit_on_std_deviation(sqrt(offset_popvar), config.N, config.pos);;
}