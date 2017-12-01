// Copyright John Maddock 2006, 2007
// Copyright Paul A. Bristow 2007

// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
using std::cout; using std::endl;
using std::left; using std::fixed; using std::right; using std::scientific;
#include <iomanip>
using std::setw;
using std::setprecision;
#include <boost/math/distributions/chi_squared.hpp>
#include <boost/math/distributions/normal.hpp>
#include <boost/math/special_functions/erf.hpp>

double upper_confidence_limit_on_std_deviation(
        double Sd,    // Sample Standard Deviation
        unsigned N,   // Sample size
        double pds)   // Probability
{
  // Calculate confidence intervals for the standard deviation.
  // For example if we set the confidence limit to
  // 0.95, we know that if we repeat the sampling
  // 100 times, then we expect that the true standard deviation
  // will be between out limits on 95 occations.
  // Note: this is not the same as saying a 95%
  // confidence interval means that there is a 95%
  // probability that the interval contains the true standard deviation.
  // The interval computed from a given sample either
  // contains the true standard deviation or it does not.
  // See http://www.itl.nist.gov/div898/handbook/eda/section3/eda358.htm

  // using namespace boost::math;
  using boost::math::chi_squared;
  using boost::math::quantile;
  using boost::math::complement;

  // Start by declaring the distribution we'll need:
  chi_squared dist(N - 1);
  
  // Calculate limits:
  double lower_limit = ((N - 1) * Sd * Sd / quantile(complement(dist, 1 - pds))); // Needs to be checked if its pds or pds/2
  double upper_limit = ((N - 1) * Sd * Sd / quantile(dist, 1 - pds));
  std::cout << "Lower Limit = " << lower_limit << "\n";
  std::cout << "Upper Limit = " << upper_limit << "\n";

  return upper_limit;
} 

double upper_confidence_limit_gaussian(
        double Sd,    // Sample Standard Deviation
        double pdv)   // Probability
{
  // Calculate confidence intervals for the standard deviation.
  // For example if we set the confidence limit to
  // 0.95, we know that if we repeat the sampling
  // 100 times, then we expect that the true standard deviation
  // will be between out limits on 95 occations.
  // Note: this is not the same as saying a 95%
  // confidence interval means that there is a 95%
  // probability that the interval contains the true standard deviation.
  // The interval computed from a given sample either
  // contains the true standard deviation or it does not.
  // See http://www.itl.nist.gov/div898/handbook/eda/section3/eda358.htm

  // using namespace boost::math;
  using boost::math::normal;
  using boost::math::quantile;
  using boost::math::complement;

  // Start by declaring the distribution we'll need:
  normal normdist(0, Sd);
  
  // Calculate limits:
  double upper_limit = quantile(normdist, pdv);

  return upper_limit;
}

double get_inverse_error_func(double probability) 
{
  // Calculate the inverse gaussian error function for a given probability
  // Check if probability value is in a valid range (absolute value)
  if(probability > 1 && probability < -1)
    return 0;

  return boost::math::erf_inv(probability);
}
