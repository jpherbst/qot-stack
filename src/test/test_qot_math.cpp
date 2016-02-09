#include <iostream>
#include <gtest/gtest.h>

#include "../qot.h"

TEST(TimelineMath, ScaleFactors) {
	timelength_t ta = ASEC(1);
	EXPECT_EQ(1ULL, ta.asec);
	timelength_t tf = FSEC(1);
	EXPECT_EQ(1000ULL, tf.asec);
	timelength_t tn = NSEC(1);
	EXPECT_EQ(1000000ULL, tn.asec);
	timelength_t tp = PSEC(1);
	EXPECT_EQ(1000000000ULL, tp.asec);
	timelength_t tu = USEC(1);
	EXPECT_EQ(1000000000000ULL, tu.asec);
	timelength_t tm = MSEC(1);
	EXPECT_EQ(1000000000000000ULL, tm.asec);
}

TEST(TimelineMath, AdditionOfSeconds) {
	timepoint_t t1 = SEC(0);
	timelength_t t2 = SEC(10);
	timepoint_add(&t1, &t2);
	EXPECT_EQ(0,t1.asec);
	EXPECT_EQ(10,t1.sec);
}

TEST(TimelineMath, SubtractionOfSeconds) {
	timepoint_t t1 = SEC(0);
	timelength_t t2 = SEC(10);
	timepoint_sub(&t1, &t2);
	EXPECT_EQ(0,t1.asec);
	EXPECT_EQ(-10,t1.sec);
}

TEST(TimelineMath, AdditionOfAttoseconds) {
	timepoint_t t1 = ASEC(0);
	timelength_t t2 = ASEC(10);
	timepoint_add(&t1, &t2);
	EXPECT_EQ(0,t1.sec);
	EXPECT_EQ(10,t1.asec);
}

TEST(TimelineMath, SubtractionOfAttoseconds) {
	timepoint_t t1 = ASEC(0);
	timelength_t t2 = ASEC(10);
	timepoint_sub(&t1, &t2);
	EXPECT_EQ(-1,t1.sec);
	EXPECT_EQ(1000000000000000000ULL-10ULL,t1.asec);
}

TEST(TimelineMath, AdditionOfSecondsAndAttoseconds) {
	timepoint_t t1 = {
		.sec  = 1,
		.asec = 1,
	};
	timelength_t t2 = {
		.sec  = 2,
		.asec = 2,
	}; 
	timepoint_add(&t1, &t2);
	EXPECT_EQ(3,t1.sec);
	EXPECT_EQ(3,t1.asec);
}

TEST(TimelineMath, SubtractionOfSecondsAndAttoseconds) {
	timepoint_t t1 = {
		.sec  = 1,
		.asec = 1,
	};
	timelength_t t2 = {
		.sec  = 2,
		.asec = 2,
	}; 
	timepoint_sub(&t1, &t2);
	EXPECT_EQ(-2,t1.sec);
	EXPECT_EQ(1000000000000000000ULL-1ULL,t1.asec);
}

TEST(TimelineMath, AddTwoLengthsOfTime) {
	timelength_t l1 = {
		.sec  = 1,
		.asec = 1,
	};
	timelength_t l2 = {
		.sec  = 2,
		.asec = 2,
	}; 
	timelength_add(&l1, &l2);
	EXPECT_EQ(3,l1.sec);
	EXPECT_EQ(3,l1.asec);
}

TEST(TimelineMath, DifferenceBetweenTimepoints) {
	timepoint_t t1 = {
		.sec  = 1,
		.asec = 1,
	};
	timepoint_t t2 = {
		.sec  = 2,
		.asec = 2,
	}; 
	timelength_t v;
	timepoint_diff(&v, &t1, &t2);
	EXPECT_EQ(1,v.sec);
	EXPECT_EQ(1,v.asec);
}