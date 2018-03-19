/**
 * @file NTP18.cpp
 * @brief Provides ntp instance based on Chrony to the sync interface
 * @author Sandeep D'souza
 * 
 * Copyright (c) Carnegie Mellon University, 2018. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *      1. Redistributions of source code must retain the above copyright notice, 
 *     this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice, 
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND f
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
 * Reference: Based on the chrony implementation of NTP
 * 1. chrony: https://chrony.tuxfamily.org/
 */

#include "NTP18.hpp"

/* ================================================== */

/* Set when the initialisation chain has been completed.  Prevents finalisation
 * chain being run if a fatal error happened early. */

static int initialised = 0;

static int exit_status = 0;

static int reload = 0;

static REF_Mode ref_mode = REF_ModeNormal;

/* ================================================== */

static void
do_platform_checks(void)
{
  /* Require at least 32-bit integers, two's complement representation and
     the usual implementation of conversion of unsigned integers */
  assert(sizeof (int) >= 4);
  assert(-1 == ~0);
  assert((int32_t)4294967295U == (int32_t)-1);
}

/* ================================================== */

static void
delete_pidfile(void)
{
  const char *pidfile = CNF_GetPidFile();

  if (!pidfile[0])
    return;

  /* Don't care if this fails, there's not a lot we can do */
  unlink(pidfile);
}

/* ================================================== */

void
MAI_CleanupAndExit(void)
{
  if (!initialised) exit(exit_status);
  
  if (CNF_GetDumpDir()[0] != '\0') {
    SRC_DumpSources();
  }

  /* Don't update clock when removing sources */
  REF_SetMode(REF_ModeIgnore);

  SMT_Finalise();
  TMC_Finalise();
  MNL_Finalise();
  CLG_Finalise();
  NSD_Finalise();
  NSR_Finalise();
  SST_Finalise();
  NCR_Finalise();
  NIO_Finalise();
  CAM_Finalise();
  KEY_Finalise();
  RCL_Finalise();
  SRC_Finalise();
  REF_Finalise();
  RTC_Finalise();
  SYS_Finalise();
  SCH_Finalise();
  LCL_Finalise();
  PRV_Finalise();

  delete_pidfile();
  
  CNF_Finalise();
  HSH_Finalise();
  LOG_Finalise();

  exit(exit_status);
}

/* ================================================== */

static void
signal_cleanup(int x)
{
  if (!initialised) exit(0);
  SCH_QuitProgram();
}

/* ================================================== */

static void
quit_timeout(void *arg)
{
  /* Return with non-zero status if the clock is not synchronised */
  exit_status = REF_GetOurStratum() >= NTP_MAX_STRATUM;
  SCH_QuitProgram();
}

/* ================================================== */

static void
ntp_source_resolving_end(void)
{
  NSR_SetSourceResolvingEndHandler(NULL);

  if (reload) {
    /* Note, we want reload to come well after the initialisation from
       the real time clock - this gives us a fighting chance that the
       system-clock scale for the reloaded samples still has a
       semblence of validity about it. */
    SRC_ReloadSources();
  }

  SRC_RemoveDumpFiles();
  RTC_StartMeasurements();
  RCL_StartRefclocks();
  NSR_StartSources();
  NSR_AutoStartSources();

  /* Special modes can end only when sources update their reachability.
     Give up immediatelly if there are no active sources. */
  if (ref_mode != REF_ModeNormal && !SRC_ActiveSources()) {
    REF_SetUnsynchronised();
  }
}

/* ================================================== */

static void
post_init_ntp_hook(void *anything)
{
  if (ref_mode == REF_ModeInitStepSlew) {
    /* Remove the initstepslew sources and set normal mode */
    NSR_RemoveAllSources();
    ref_mode = REF_ModeNormal;
    REF_SetMode(ref_mode);
  }

  /* Close the pipe to the foreground process so it can exit */
  LOG_CloseParentFd();

  CNF_AddSources();
  CNF_AddBroadcasts();

  NSR_SetSourceResolvingEndHandler(ntp_source_resolving_end);
  NSR_ResolveSources();
}

/* ================================================== */

static void
reference_mode_end(int result)
{
  switch (ref_mode) {
    case REF_ModeNormal:
    case REF_ModeUpdateOnce:
    case REF_ModePrintOnce:
      exit_status = !result;
      SCH_QuitProgram();
      break;
    case REF_ModeInitStepSlew:
      /* Switch to the normal mode, the delay is used to prevent polling
         interval shorter than the burst interval if some configured servers
         were used also for initstepslew */
      SCH_AddTimeoutByDelay(2.0, post_init_ntp_hook, NULL);
      break;
    default:
      assert(0);
  }
}

/* ================================================== */

static void
post_init_rtc_hook(void *anything)
{
  if (CNF_GetInitSources() > 0) {
    CNF_AddInitSources();
    NSR_StartSources();
    assert(REF_GetMode() != REF_ModeNormal);
    /* Wait for mode end notification */
  } else {
    (post_init_ntp_hook)(NULL);
  }
}

/* ================================================== */

static void
check_pidfile(void)
{
  const char *pidfile = CNF_GetPidFile();
  FILE *in;
  int pid, count;
  
  in = fopen(pidfile, "r");
  if (!in)
    return;

  count = fscanf(in, "%d", &pid);
  fclose(in);
  
  if (count != 1)
    return;

  if (getsid(pid) < 0)
    return;

  LOG_FATAL("Another chronyd may already be running (pid=%d), check %s",
            pid, pidfile);
}

/* ================================================== */

static void
write_pidfile(void)
{
  const char *pidfile = CNF_GetPidFile();
  FILE *out;

  if (!pidfile[0])
    return;

  out = fopen(pidfile, "w");
  if (!out) {
    LOG_FATAL("Could not open %s : %s", pidfile, strerror(errno));
  } else {
    fprintf(out, "%d\n", (int)getpid());
    fclose(out);
  }
}

/* ================================================== */

static void
go_daemon(void)
{
  int pid, fd, pipefd[2];

  /* Create pipe which will the daemon use to notify the grandparent
     when it's initialised or send an error message */
  if (pipe(pipefd)) {
    LOG_FATAL("pipe() failed : %s", strerror(errno));
  }

  /* Does this preserve existing signal handlers? */
  pid = fork();

  if (pid < 0) {
    LOG_FATAL("fork() failed : %s", strerror(errno));
  } else if (pid > 0) {
    /* In the 'grandparent' */
    char message[1024];
    int r;

    close(pipefd[1]);
    r = read(pipefd[0], message, sizeof (message));
    if (r) {
      if (r > 0) {
        /* Print the error message from the child */
        message[sizeof (message) - 1] = '\0';
        fprintf(stderr, "%s\n", message);
      }
      exit(1);
    } else
      exit(0);
  } else {
    close(pipefd[0]);

    setsid();

    /* Do 2nd fork, as-per recommended practice for launching daemons. */
    pid = fork();

    if (pid < 0) {
      LOG_FATAL("fork() failed : %s", strerror(errno));
    } else if (pid > 0) {
      exit(0); /* In the 'parent' */
    } else {
      /* In the child we want to leave running as the daemon */

      /* Change current directory to / */
      if (chdir("/") < 0) {
        LOG_FATAL("chdir() failed : %s", strerror(errno));
      }

      /* Don't keep stdin/out/err from before. But don't close
         the parent pipe yet. */
      for (fd=0; fd<1024; fd++) {
        if (fd != pipefd[1])
          close(fd);
      }

      LOG_SetParentFd(pipefd[1]);
    }
  }
}

/* ================================================== */

static void
print_help(const char *progname)
{
      printf("Usage: %s [-4|-6] [-n|-d] [-q|-Q] [-r] [-R] [-s] [-t TIMEOUT] [-f FILE|COMMAND...]\n",
             progname);
}

/* ================================================== */

static void
print_version(void)
{
  printf("chronyd (chrony) version %s (%s)\n", CHRONY_VERSION, CHRONYD_FEATURES);
}

/* ================================================== */

static int
parse_int_arg(const char *arg)
{
  int i;

  if (sscanf(arg, "%d", &i) != 1)
    LOG_FATAL("Invalid argument %s", arg);
  return i;
}

using namespace qot;

NTP18::NTP18(boost::asio::io_service *io, // ASIO handle
	const std::string &iface,     // interface	
	struct uncertainty_params config // uncertainty calculation configuration		
	) : asio(io), baseiface(iface), sync_uncertainty(config)
{	
	this->Reset();
}

NTP18::~NTP18()
{
	this->Stop();
}

void NTP18::Reset()
{
	
}

void NTP18::Start(
	bool master,
	int log_sync_interval,
	uint32_t sync_session,
	int timelineid,
	int *timelinesfd,
	uint16_t timelines_size)
{
	// First stop any sync that is currently underway
	this->Stop();

	// Restart sync
	BOOST_LOG_TRIVIAL(info) << "Starting NTP synchronization";
	kill = false;

	// Initialize Local Tracking Variable for Clock-Skew Statistics (Checks Staleness)
	last_clocksync_data_point.offset  = 0;
	last_clocksync_data_point.drift   = 0;
	last_clocksync_data_point.data_id = 0;

	// Initialize Global Variable for Clock-Skew Statistics 
	ntp_clocksync_data_point.offset  = 0;
	ntp_clocksync_data_point.drift   = 0;
	ntp_clocksync_data_point.data_id = 0;

	thread = boost::thread(boost::bind(&NTP18::SyncThread, this, timelinesfd, timelines_size));
}

void NTP18::Stop()
{
	BOOST_LOG_TRIVIAL(info) << "Stopping NTP synchronization ";
	kill = true;
	thread.join();
}


int NTP18::SyncThread(int *timelinesfd, uint16_t timelines_size)
{
	BOOST_LOG_TRIVIAL(info) << "Sync thread started";

	const char *conf_file = DEFAULT_CONF_FILE;
    const char *progname = argv[0];
    char *user = NULL, *log_file = NULL;
    struct passwd *pw;
    int opt, debug = 0, nofork = 0, address_family = IPADDR_UNSPEC;
    int do_init_rtc = 0, restarted = 0, client_only = 0, timeout = 0;
    int scfilter_level = 0, lock_memory = 0, sched_priority = 0;
    int clock_control = 1, system_log = 1;
    int config_args = 0;

  	do_platform_checks();

  	LOG_Initialise();

  	optind = 1;

    if (getuid() && !client_only)
      LOG_FATAL("Not superuser");

    if (log_file) {
      LOG_OpenFileLog(log_file);
    } else if (system_log) {
      LOG_OpenSystemLog();
    }
  
    LOG_SetDebugLevel(debug);
  
    LOG(LOGS_INFO, "chronyd version %s starting (%s)", CHRONY_VERSION, CHRONYD_FEATURES);

    DNS_SetAddressFamily(address_family);

    CNF_Initialise(restarted, client_only);

    /* Check whether another chronyd may already be running */
    check_pidfile();

    /* Write our pidfile to prevent other chronyds running */
    write_pidfile();

    PRV_Initialise();
    LCL_Initialise();
    SCH_Initialise();
    SYS_Initialise(clock_control);
    RTC_Initialise(do_init_rtc);
    SRC_Initialise();
    RCL_Initialise();
    KEY_Initialise();

    /* Open privileged ports before dropping root */
    CAM_Initialise(address_family);
    NIO_Initialise(address_family);
    NCR_Initialise();
    CNF_SetupAccessRestrictions();

    /* Command-line switch must have priority */
    if (!sched_priority) {
      sched_priority = CNF_GetSchedPriority();
    }
    if (sched_priority) {
      SYS_SetScheduler(sched_priority);
    }

    if (lock_memory || CNF_GetLockMemory()) {
      SYS_LockMemory();
    }

    if (!user) {
      user = CNF_GetUser();
    }

    if ((pw = getpwnam(user)) == NULL)
      LOG_FATAL("Could not get %s uid/gid", user);

    /* Create all directories before dropping root */
    CNF_CreateDirs(pw->pw_uid, pw->pw_gid);

    /* Drop root privileges if the specified user has a non-zero UID */
    if (!geteuid() && (pw->pw_uid || pw->pw_gid))
      SYS_DropRoot(pw->pw_uid, pw->pw_gid);

    REF_Initialise();
    SST_Initialise();
    NSR_Initialise();
    NSD_Initialise();
    CLG_Initialise();
    MNL_Initialise();
    TMC_Initialise();
    SMT_Initialise();

    /* From now on, it is safe to do finalisation on exit */
    initialised = 1;

    UTI_SetQuitSignalsHandler(signal_cleanup);

    CAM_OpenUnixSocket();

    if (scfilter_level)
      SYS_EnableSystemCallFilter(scfilter_level);

    if (ref_mode == REF_ModeNormal && CNF_GetInitSources() > 0) {
      ref_mode = REF_ModeInitStepSlew;
    }

    REF_SetModeEndHandler(reference_mode_end);
    REF_SetMode(ref_mode);

    if (timeout > 0)
      SCH_AddTimeoutByDelay(timeout, quit_timeout, NULL);

    if (do_init_rtc) {
      RTC_TimeInit(post_init_rtc_hook, NULL);
    } else {
      post_init_rtc_hook(NULL);
    }

    /* The program normally runs under control of the main loop in
       the scheduler. */
    SCH_MainLoop();

    LOG(LOGS_INFO, "chronyd exiting");

    MAI_CleanupAndExit();

    return 0;
	
}
