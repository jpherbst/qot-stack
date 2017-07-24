/*
 * Local NTP (LNTP)
 * Author: Fatima Anwar, Zhou Fang
 * Reference: 
 * 1. NTPv4: https://www.eecis.udel.edu/~mills/database/reports/ntp4/ntp4.pdf
 * 2. Use some source code from: http://blog.csdn.net/rich_baba/article/details/6052863
 */

#define  NR_REMOTE	5         // Nr Time Master
#define  DEF_NTP_SERVER0  "pool.ntp.org"
#define  DEF_NTP_SERVER1  "0.us.pool.ntp.org"
#define  DEF_NTP_SERVER2  "1.us.pool.ntp.org"
#define  DEF_NTP_SERVER3  "2.us.pool.ntp.org"
#define  DEF_NTP_SERVER4  "3.us.pool.ntp.org"

#define  MIN_NTP_SAMPLE  3    // minimum required NTP samples
#define  N_MIN_SURVIVOR 3     // minimum required survivosr
#define  NSTAGE NR_REMOTE

#define  NUM_SAMPLE  10  // 1h /per 1s
#define  FILE_PATH   "result.txt"

// NTPv4 specification
#define RATIO_SCALE 0.8

// Used for debugging
#define DEBUG FALSE
#define CLOCK_REAL FALSE

#define  DEF_PSEC        1  // Period of NTP sync
#define  DEF_TIMEOUT     1  // ntp packet timeout (s)

// local clock parameters
#define  PREC -10       // local clock precision ~1ms
#define  DRIFT 100e-6   // local clock drift (max)

#define  MINDISP 1000     // minimum dispersion (us)
#define  MAXDISP 16000000 // maximum dispersion (us)
#define  MAXDIST 1000000  // maximum distance of NTP server per stratum

 #define MAXFREQADJ 100000000 // 1e8, 100 msec / sec
