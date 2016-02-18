#include <iostream>
#include <gtest/gtest.h>

extern "C" {
    #include "../qot_types.h"
}

TEST(TimelineMath, TL_FROM) {
	timelength_t t;
    TL_FROM_SEC(t,1ULL);
    EXPECT_EQ(t.sec, 1ULL);
    EXPECT_EQ(t.asec,0ULL);
    TL_FROM_mSEC(t,1001ULL);
    EXPECT_EQ(t.sec, 1ULL);
    EXPECT_EQ(t.asec,1000000000000000ULL);
    TL_FROM_uSEC(t,1000001ULL);
    EXPECT_EQ(t.sec, 1ULL);
    EXPECT_EQ(t.asec,1000000000000ULL);
    TL_FROM_nSEC(t,1000000001ULL);
    EXPECT_EQ(t.sec, 1ULL);
    EXPECT_EQ(t.asec,1000000000ULL);
    TL_FROM_pSEC(t,1000000000001ULL);
    EXPECT_EQ(t.sec, 1ULL);
    EXPECT_EQ(t.asec,1000000ULL);
    TL_FROM_fSEC(t,1000000000000001ULL);
    EXPECT_EQ(t.sec, 1ULL);
    EXPECT_EQ(t.asec,1000ULL);
    TL_FROM_aSEC(t,1000000000000000001ULL);
    EXPECT_EQ(t.sec, 1ULL);
    EXPECT_EQ(t.asec,1ULL);
}

TEST(TimelineMath, TL_TO) {
    u64 v;
    timelength_t t;
    TL_FROM_SEC(t,1ULL);
    v = TL_TO_SEC(t);
    EXPECT_EQ(v,1LL);
    TL_FROM_mSEC(t,1ULL);
    v = TL_TO_mSEC(t);
    EXPECT_EQ(v,1LL);
    TL_FROM_uSEC(t,1ULL);
    v = TL_TO_uSEC(t);
    EXPECT_EQ(v,1LL);
    TL_FROM_nSEC(t,1ULL);
    v = TL_TO_nSEC(t);
    EXPECT_EQ(v,1LL);
    TL_FROM_pSEC(t,1ULL);
    v = TL_TO_pSEC(t);
    EXPECT_EQ(v,1LL);
    TL_FROM_fSEC(t,1ULL);
    v = TL_TO_fSEC(t);
    EXPECT_EQ(v,1LL);
    TL_FROM_aSEC(t,1ULL);
    v = TL_TO_aSEC(t);
    EXPECT_EQ(v,1LL);
}

TEST(TimelineMath, timelength_add) {
	 timelength_t t1, t2;
	 TL_FROM_SEC(t1,1ULL);
	 TL_FROM_aSEC(t2,1ULL);
	 timelength_add(&t1,&t2);
	 EXPECT_EQ(t1.sec,1LL);
	 EXPECT_EQ(t1.asec,1LL);
}

TEST(TimelineMath, timelength_cmp) {
	int v;
	timelength_t l1, l2;
	TL_FROM_mSEC(l1,1ULL);
	TL_FROM_mSEC(l2,1ULL);
	v = timelength_cmp(&l1, &l2);
	EXPECT_EQ(0,v);
	TL_FROM_mSEC(l1,1ULL);
	TL_FROM_mSEC(l2,2ULL);
	v = timelength_cmp(&l1, &l2);
	EXPECT_EQ(1,v);
	TL_FROM_mSEC(l1,2ULL);
	TL_FROM_mSEC(l2,1ULL);
	v = timelength_cmp(&l1, &l2);
	EXPECT_EQ(-1,v);
}

TEST(TimelineMath, timelength_min) {
	int v;
	timelength_t l, l1, l2;
	TL_FROM_SEC(l1,1ULL);
	TL_FROM_SEC(l2,2ULL);
	timelength_min(&l, &l1, &l2);
	EXPECT_EQ(1ULL,l.sec);
	TL_FROM_SEC(l1,2ULL);
	TL_FROM_SEC(l2,1ULL);
	timelength_min(&l, &l1, &l2);
	EXPECT_EQ(1ULL,l.sec);
	TL_FROM_SEC(l1,1ULL);
	TL_FROM_SEC(l2,1ULL);
	timelength_min(&l, &l1, &l2);
	EXPECT_EQ(1ULL,l.sec);
}

TEST(TimelineMath, timelength_max) {
	int v;
	timelength_t l, l1, l2;
	TL_FROM_SEC(l1,1ULL);
	TL_FROM_SEC(l2,2ULL);
	timelength_max(&l, &l1, &l2);
	EXPECT_EQ(2ULL,l.sec);
	TL_FROM_SEC(l1,2ULL);
	TL_FROM_SEC(l2,1ULL);
	timelength_max(&l, &l1, &l2);
	EXPECT_EQ(2ULL,l.sec);
	TL_FROM_SEC(l1,2ULL);
	TL_FROM_SEC(l2,2ULL);
	timelength_max(&l, &l1, &l2);
	EXPECT_EQ(2ULL,l.sec);
}

TEST(TimelineMath, TP_FROM) {
    timepoint_t t;
    TP_FROM_SEC(t,-1LL);
    EXPECT_EQ(t.sec, -1LL);
    EXPECT_EQ(t.asec,0LL);
    TP_FROM_mSEC(t,-1001LL);
    EXPECT_EQ(t.sec,  -2LL);
    EXPECT_EQ(t.asec, 999000000000000000ULL);
    TP_FROM_uSEC(t,-1000001LL);
    EXPECT_EQ(t.sec,  -2LL);
    EXPECT_EQ(t.asec, 999999000000000000ULL);
    TP_FROM_nSEC(t,-1000000001LL);
    EXPECT_EQ(t.sec,  -2LL);
    EXPECT_EQ(t.asec, 999999999000000000ULL);
    TP_FROM_pSEC(t,-1000000000001LL);
    EXPECT_EQ(t.sec,  -2LL);
    EXPECT_EQ(t.asec, 999999999999000000ULL);
    TP_FROM_fSEC(t,-1000000000000001LL);
    EXPECT_EQ(t.sec,  -2LL);
    EXPECT_EQ(t.asec, 999999999999999000ULL);
    TP_FROM_aSEC(t,-1000000000000000001LL);
    EXPECT_EQ(t.sec,  -2LL);
    EXPECT_EQ(t.asec, 999999999999999999ULL);
}

TEST(TimelineMath, TP_TO) {
    u64 v;
    timepoint_t t;
    TP_FROM_SEC(t,-1LL);
    v = TP_TO_SEC(t);
    EXPECT_EQ(v,-1LL);
    TP_FROM_mSEC(t,-1LL);
    v = TP_TO_mSEC(t);
    EXPECT_EQ(v,-1LL);
    TP_FROM_uSEC(t,-1LL);
    v = TP_TO_uSEC(t);
    EXPECT_EQ(v,-1LL);
    TP_FROM_nSEC(t,-1LL);
    v = TP_TO_nSEC(t);
    EXPECT_EQ(v,-1LL);
    TP_FROM_pSEC(t,-1LL);
    v = TP_TO_pSEC(t);
    EXPECT_EQ(v,-1LL);
    TP_FROM_fSEC(t,-1LL);
    v = TP_TO_fSEC(t);
    EXPECT_EQ(v,-1LL);
    TP_FROM_aSEC(t,-1LL);
    v = TP_TO_aSEC(t);
    EXPECT_EQ(v,-1LL);
}

TEST(TimelineMath, timepoint_add) {
    timepoint_t tp;
    timelength_t tl;
    TP_FROM_SEC(tp,0LL);
	TL_FROM_SEC(tl,10LL);
	timepoint_add(&tp, &tl);
	EXPECT_EQ(10,tp.sec);
	EXPECT_EQ(0,tp.asec);
    TP_FROM_aSEC(tp,0LL);
	TL_FROM_aSEC(tl,10LL);
	timepoint_add(&tp, &tl);
	EXPECT_EQ(0,tp.sec);
	EXPECT_EQ(10,tp.asec);
    TP_FROM_mSEC(tp,1001LL);
	TL_FROM_mSEC(tl,1001LL);
	timepoint_add(&tp, &tl);
	EXPECT_EQ(2,tp.sec);
	EXPECT_EQ(2000000000000000ULL,tp.asec);
}

TEST(TimelineMath, timepoint_sub) {
    timepoint_t tp;
    timelength_t tl;
    TP_FROM_SEC(tp,0LL);
	TL_FROM_SEC(tl,10LL);
	timepoint_sub(&tp, &tl);
	EXPECT_EQ(-10,tp.sec);
	EXPECT_EQ(0,tp.asec);
    TP_FROM_aSEC(tp,0LL);
	TL_FROM_aSEC(tl,1LL);
	timepoint_sub(&tp, &tl);
	EXPECT_EQ(-1,tp.sec);
	EXPECT_EQ(999999999999999999ULL,tp.asec);
    TP_FROM_mSEC(tp,0LL);
	TL_FROM_mSEC(tl,1001LL);
	timepoint_sub(&tp, &tl);
	EXPECT_EQ(-2,tp.sec);
	EXPECT_EQ(999000000000000000ULL,tp.asec);
}

TEST(TimelineMath, timepoint_diff) {
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

TEST(TimelineMath, timepoint_cmp) {
	int v;
	timepoint_t t1, t2;
	TP_FROM_mSEC(t1, 1LL);
	TP_FROM_mSEC(t2, 1LL);
	v = timepoint_cmp(&t1, &t2);
	EXPECT_EQ(0,v);
	TP_FROM_mSEC(t1,-1LL);
	TP_FROM_mSEC(t2, 1LL);
	v = timepoint_cmp(&t1, &t2);
	EXPECT_EQ(1,v);
	TP_FROM_mSEC(t1, 1LL);
	TP_FROM_mSEC(t2,-1LL);
	v = timepoint_cmp(&t1, &t2);
	EXPECT_EQ(-1,v);
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


*/