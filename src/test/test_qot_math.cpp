#include <iostream>
#include <gtest/gtest.h>

extern "C" {
    #include "../qot_types.h"
}

TEST(TimelineMath, TimeLengthInitializers) {
	timelength_t t;
    TL_SEC(t,1ULL);
    EXPECT_EQ(t.sec, 1ULL);
    EXPECT_EQ(t.asec,0ULL);
    TL_mSEC(t,1001ULL);
    EXPECT_EQ(t.sec, 1ULL);
    EXPECT_EQ(t.asec,1000000000000000ULL);
    TL_uSEC(t,1000001ULL);
    EXPECT_EQ(t.sec, 1ULL);
    EXPECT_EQ(t.asec,1000000000000ULL);
    TL_pSEC(t,1000000001ULL);
    EXPECT_EQ(t.sec, 1ULL);
    EXPECT_EQ(t.asec,1000000000ULL);
    TL_nSEC(t,1000000000001ULL);
    EXPECT_EQ(t.sec, 1ULL);
    EXPECT_EQ(t.asec,1000000ULL);
    TL_fSEC(t,1000000000000001ULL);
    EXPECT_EQ(t.sec, 1ULL);
    EXPECT_EQ(t.asec,1000ULL);
    TL_aSEC(t,1000000000000000001ULL);
    EXPECT_EQ(t.sec, 1ULL);
    EXPECT_EQ(t.asec,1ULL);
}

TEST(TimelineMath, TimePointInitializers) {
    timepoint_t t;
    //TP_SEC(t,-1LL);
    //EXPECT_EQ(t.sec, -1LL);
    //EXPECT_EQ(t.asec,0LL);
    TP_mSEC(t,-1001LL);
    EXPECT_EQ(t.sec,  -2LL);
    EXPECT_EQ(t.asec, aSEC_PER_SEC-mSEC_PER_SEC);
    /*
    EXPECT_EQ(t.asec,aSEC_PER_SEC-1000000000000000LL);
    TP_uSEC(t,-1000001LL);
    EXPECT_EQ(t.sec, -2LL);
    EXPECT_EQ(t.asec,aSEC_PER_SEC-1000000000000LL);
    TP_pSEC(t,-1000000001LL);
    EXPECT_EQ(t.sec, -2LL);
    EXPECT_EQ(t.asec,aSEC_PER_SEC-1000000000LL);
    TP_nSEC(t,-1000000000001LL);
    EXPECT_EQ(t.sec, -2LL);
    EXPECT_EQ(t.asec,aSEC_PER_SEC-1000000LL);
    TP_fSEC(t,-1000000000000001LL);
    EXPECT_EQ(t.sec, -2LL);
    EXPECT_EQ(t.asec,aSEC_PER_SEC-1000LL);
    TP_aSEC(t,-1000000000000000001LL);
    EXPECT_EQ(t.sec, -2LL);
    EXPECT_EQ(t.asec,aSEC_PER_SEC-1LL);
    */
}

/*
TEST(TimelineMath, FrequencyInitializers) {
    frequency_t tE = EHz(1);
    EXPECT_EQ(1000000000000000000ULL, tE.hz);
    EXPECT_EQ(0ULL, tE.ahz);
    frequency_t tP = PHz(1);
    EXPECT_EQ(1000000000000000ULL, tP.hz);
    EXPECT_EQ(0ULL, tP.ahz);
    frequency_t tT = THz(1);
    EXPECT_EQ(1000000000000ULL, tT.hz);
    EXPECT_EQ(0ULL, tT.ahz);
    frequency_t tG = GHz(1);
    EXPECT_EQ(1000000000ULL, tG.hz);
    EXPECT_EQ(0ULL, tG.ahz);
    frequency_t tM = MHz(1);
    EXPECT_EQ(1000000ULL, tM.hz);
    EXPECT_EQ(0ULL, tM.ahz);
    frequency_t tK = KHz(1);
    EXPECT_EQ(1000ULL, tK.hz);
    EXPECT_EQ(0ULL, tK.ahz);
    frequency_t t = Hz(1);
    EXPECT_EQ(1ULL, t.hz);
    EXPECT_EQ(0ULL, t.ahz);
    frequency_t ta = aHz(1);
    EXPECT_EQ(1ULL, ta.ahz);
    EXPECT_EQ(0ULL, ta.hz);
    frequency_t tf = fHz(1);
    EXPECT_EQ(1000ULL, tf.ahz);
    EXPECT_EQ(0ULL, ta.hz);
    frequency_t tn = nHz(1);
    EXPECT_EQ(1000000ULL, tn.ahz);
    EXPECT_EQ(0ULL, ta.hz);
    frequency_t tp = pHz(1);
    EXPECT_EQ(1000000000ULL, tp.ahz);
    EXPECT_EQ(0ULL, ta.hz);
    frequency_t tu = uHz(1);
    EXPECT_EQ(1000000000000ULL, tu.ahz);
    EXPECT_EQ(0ULL, ta.hz);
    frequency_t tm = mHz(1);
    EXPECT_EQ(1000000000000000ULL, tm.ahz);
    EXPECT_EQ(0ULL, ta.hz);
}

TEST(TimelineMath, PowerInitializers) {
    power_t tE = EWATT(1);
    EXPECT_EQ(1000000000000000000ULL, tE.watt);
    EXPECT_EQ(0ULL, tE.awatt);
    power_t tP = PWATT(1);
    EXPECT_EQ(1000000000000000ULL, tP.watt);
    EXPECT_EQ(0ULL, tP.awatt);
    power_t tT = TWATT(1);
    EXPECT_EQ(1000000000000ULL, tT.watt);
    EXPECT_EQ(0ULL, tT.awatt);
    power_t tG = GWATT(1);
    EXPECT_EQ(1000000000ULL, tG.watt);
    EXPECT_EQ(0ULL, tG.awatt);
    power_t tM = MWATT(1);
    EXPECT_EQ(1000000ULL, tM.watt);
    EXPECT_EQ(0ULL, tM.awatt);
    power_t tK = KWATT(1);
    EXPECT_EQ(1000ULL, tK.watt);
    EXPECT_EQ(0ULL, tK.awatt);
    power_t t = WATT(1);
    EXPECT_EQ(1ULL, t.watt);
    EXPECT_EQ(0ULL, t.awatt);
    power_t ta = aWATT(1);
    EXPECT_EQ(1ULL, ta.awatt);
    EXPECT_EQ(0ULL, ta.watt);
    power_t tf = fWATT(1);
    EXPECT_EQ(1000ULL, tf.awatt);
    EXPECT_EQ(0ULL, ta.watt);
    power_t tn = nWATT(1);
    EXPECT_EQ(1000000ULL, tn.awatt);
    EXPECT_EQ(0ULL, ta.watt);
    power_t tp = pWATT(1);
    EXPECT_EQ(1000000000ULL, tp.awatt);
    EXPECT_EQ(0ULL, ta.watt);
    power_t tu = uWATT(1);
    EXPECT_EQ(1000000000000ULL, tu.awatt);
    EXPECT_EQ(0ULL, ta.watt);
    power_t tm = mWATT(1);
    EXPECT_EQ(1000000000000000ULL, tm.awatt);
    EXPECT_EQ(0ULL, ta.watt);
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
	timepoint_t t1 = aSEC(0);
	timelength_t t2 = aSEC(10);
	timepoint_add(&t1, &t2);
	EXPECT_EQ(0,t1.sec);
	EXPECT_EQ(10,t1.asec);
}

TEST(TimelineMath, SubtractionOfAttoseconds) {
	timepoint_t t1 = aSEC(0);
	timelength_t t2 = aSEC(10);
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

*/