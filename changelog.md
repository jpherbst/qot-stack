- Simple user scripts to uild sd card images

- Initial commit for refactored qot-stack code

- Updated udev rules to automagically create a symbolic link /dev/ptp_core and /dev/ptp_cpts to the core and Ethernet timer

- Fixed some bugs -- in particular an overflow issue for periods > 2 seconds and the mult/shift values for the core timer.

- Various memory leak fixes

- Fixed a few bugs. L1 sync now works with < 10ns error :)
$> capes ROSELINE-QOT
$> ph2phc

- Added a first stab at phc2phc code.

- Added some phc2phc code to pull timestamps from the system and network PHCs

- Start time is now deterministic too!

- For some infuriating reason, device treestraverse child nodes in reverse order. To keep 1-1 channel matching between the am335x and timing pins I've modified the orderin of child pins in the device overlay.

- Programmable output now works 50/50 duty cycle from 1us to 10s, but the deterministic starting point is best effort.

- External timestamps and programmable interrupts can be configured on the OMAP timers. Next step is to convert the PTP request to counter values, and fix dynamic pinmuxing.

- Added prescaler to timer config

- Slightly more progress towards a PHC interface to the AM335x timer subsystem

- Committed for backup

- Committed for backup

- Step in the right direction (hopefully)

- Before testing PTP

- Working commit without qotdaemon checked

- Module Compiles

- Sandeep : Merging with ANdrew's code compiles

- Merged in asymingt/qot-stack (pull request #7)
Bringing master up to date with my work in progress code

- Probably need to map TX timestamps into global reference frame :)

- Trying E2E software timestamping as an alternative

- Trying E2E software timestamping as an alternative

- Trying E2E software timestamping as an alternative

- Trying E2E software timestamping as an alternative

- Trying E2E software timestamping as an alternative

- Trying E2E software timestamping as an alternative

- Trying E2E software timestamping as an alternative

- Trying E2E software timestamping as an alternative

- Trying E2E software timestamping as an alternative

- Trying E2E software timestamping as an alternative

- Trying E2E software timestamping as an alternative

- Trying E2E software timestamping as an alternative

- Trying E2E software timestamping as an alternative

- Trying E2E software timestamping as an alternative

- Trying E2E software timestamping as an alternative

- Fixed mult and shift values

- Fixed mult and shift values

- Bumped up sync intensity of test

- Bumped up sync intensity of test

- Check to see if the discipline is actually happening

- OK, so the mechanism is functional but the loop doesn't close. I need to give some thought as to why this is the case. Bumped down debug messages for the time being.

- Silly mistake.

- Modified kernel module to apply projection

- Modified linuxptp to apply the projection given by our timeline to incoming hardware timestamps.

- Small modification to sk.c to print timestamps...

- Bumped down message intensity

- Bumped down message intensity

- Better random numbers over multiple platforms

- Added eth0 as a config option and restart PTP whenever a domain/master/slave/accuracy change happens.

- Added eth0 as a config option and restart PTP whenever a domain/master/slave/accuracy change happens.

- Small improvements to coordination algorithm

- Small improvements to coordination algorithm

- Small improvements to coordination algorithm

- Small improvements to coordination algorithm

- Some more intense debugging output

- Removed the iface phc_index check

- Merged rose-line/qot-stack into master

- readme.md edited online with Bitbucket

- Merged in asymingt/qot-stack (pull request #6)
Switch to PTP clocks and semi-working L1-sync

- First stab at PTP implementation.

- First stab at PTP implementation.

- First stab at PTP implementation.

- First stab at PTP implementation.

- First stab at PTP implementation.

- Fixed a problem where the system service was not notified of events, because of its binding not being in the res or acc linked list.

- Merged rose-line/qot-stack into master

- Switched to using the ptp clock type, rather than the posix clock type. All metadata about the PTP clock is delivered alongside the clock /dev/qot. There is now a single poll channel for events per binding.

- readme.md edited online with Bitbucket

- Merged rose-line/qot-stack into master

- Merged in asymingt/qot-stack/synchronization_service (pull request #5)
Started the coordinator / synchronization engines. Mixing C/C++ code is going to be a bit tricky here. Patched linuxptp 1.5 in a few ways top enable compilation.

- Started the coordinator / synchronization engines. Mixing C/C++ code is going to be a bit tricky here. Patched linuxptp 1.5 in a few ways top enable compilation.

- Merged rose-line/qot-stack into master

- readme.md edited online with Bitbucket

- readme.md edited online with Bitbucket

- Merged rose-line/qot-stack into master

- Merged in asymingt/qot-stack/synchronization_service (pull request #4)
Moved the module code to be inline with install instructions

- Moved the module code to be inline with install instructions

- Merged in asymingt/qot-stack/synchronization_service (pull request #3)
Small fix to prevent compilation error

- Small fix to prevent compilatione error

- Merged rose-line/qot-stack into master

- readme.md edited online with Bitbucket

- readme.md edited online with Bitbucket

- readme.md edited online with Bitbucket

- readme.md edited online with Bitbucket

- Merged in asymingt/qot-stack (pull request #2)
Master

- readme.md edited online with Bitbucket

- readme.md edited online with Bitbucket

- Merged rose-line/qot-stack into master

- Merged in asymingt/qot-stack (pull request #1)
Various small fixes for initial distribution

- Removed old install scripts and extra files that are now no longer required.

- Small fixes to PTP code

- Updated the modules

- Removed thirdparty submodules that aren't really used

- Updated some readme instructions

- Updated some readme instructions

- Updated some readme instructions

- Updated some readme instructions

- Updated some readme instructions

- Updated some readme instructions

- Updated some readme instructions

- Updated some readme instructions

- Updated some readme instructions

- Updated some readme instructions

- Updated some readme instructions

- Updated some readme instructions

- Updated some readme instructions

- readme.md edited online with Bitbucket

- Bumped down to linuxptp 1.5 and started to setup a template for the sync code

- Managed to get the starting point of a DDS service working. There is still a segfault due to some POSIX clock issue in the kernel, which I need to resolve. Next step is to buidl very simple corodination code in which a PTP domain is used to carry out sync.

- Had a nice fight with () C/C++ namespace issues, and (2) cmake IDL conditional regeneration this morning. Also, added a cmake uninstall target.

- Had a nice fight with () C/C++ namespace issues, and (2) cmake IDL conditional regeneration this morning. Also, added a cmake uninstall target.

- Started experimenting with DDS

- Started experimenting with DDS

- readme.md edited online with Bitbucket

- Added updated CMakeLists

- Finally a working cross-compiler for both opensplice and boost.

- Finally a working cross-compiler for both opensplice and boost.

- Finally a working cross-compiler for both opensplice and boost.

- Changed kernel module to use the NFS-exported rootfs on the NTB system

- Sync code now looks for /dev/timelineX files

- Sync code now looks for /dev/timelineX files

- Updated buld script to use kernel cross-compiler

- Added decawave and atmel device tree overlays for the QoT stack in preparation for diffrent driversets

- Some bugfixes to enable compare mode. Projection from remote timeline to core time is still required.

- Removed the workqueue from the ISR as it was causing OOPS on fast incoming capture events.

- Removed the debugging log spam from qot_core.

- Capture poll events now project into global timeline at userspace. Testing process. 1. capes ROSELINE-qot 2. /usr/local/bin/helloworld_capture
(now touch P8.8 to the 3.3v line and you should see a poll event with time)

- Capture poll events now project into global timeline at userspace. Testing process. 1. capes ROSELINE-qot 2. /usr/local/bin/helloworld_capture
(now touch P8.8 to the 3.3v line and you should see a poll event with time)

- Added some more event identifiers and basic (untested) clock disciplining.

- Fixed a critical error in the device tree overlay in which I setup the pinmux as an input yet passed an output (capture) to the driver configuration. This caused a kernel OOPS on loading. Sorry! Also, the modules ar enow placed int he misc directory and I 've supressed a compiler warning about const char*.

- Commit for adwait

- Updated kernel config for adwait

- Improved helloworld test to allow specification of iterations, timeline name and binding name. Tested creation of parallel timelines and it works. All that needs to be finalized now is the actual disciplining mechanism, and the projection to and from local/global time in capture/compare events.

- Asynchronous capture via polling now works

- Various fixed. Core timecounter now starts up and correctly counts "nanoseconds" from 0, assuming that it's clock runs perfectly.

- Added the pwm_test.c code for Justin and fixed the IRQ issue causign the OOPS int he qot_am335x driver.

- Various bugfixes and the creation of a capes shorthand script for loading and unloading of capes. Eg. the bash command "capes" lists all slots, and "capes ROSELINE-QOT" loads the QoT cape and "capes -x" (where x is the slot number with the cape) unloads a cape. QoT module still segfaults; busy debugging...

- Clean up timers, bindings and timelines properly!

- Added udev install script

- Added a netboot installation script and configuration file for 4.1.12-bone-rt-r16.

- Added Robert Nelson's overlay tools

- Added Robert Nelson's overlay tools

- Some changes to the device tree overlay and some fixes to the platform driver

- Added a device tree overlay to setup the QoT stack. The qot_core module seems to insmod without an error, but there is an OOOPS on the qot_am335x. Also, I need to correctly implemement the timer discipling and core <-> timeline mappings.

- Swung back to the unified QoT core, as it was getting difficult to preserve the timeline to clock and binding to ioctl relationship. Code now compiles, but is not tested. I will need to debug for OOOPS in the upcoming week.

- Saved changes prior to a potentially difficult refactor

- I hope Fatima doesn't kill me for these API changes...

- Added a poll test for Adwait to look at

- Changed API to provide polling-based capture / compare, and adapted the helloworld examples accordingly.

- Changed API to provide polling-based capture / compare, and adapted the helloworld examples accordingly.

- Extended qot module to allow parallel access from userspace by multiple API invocations (file descriptors). A poll function blocks each decriptor until some event (input capture) has occurred. I still need to implement the actual hardware capture itself.

- Converted timeline engine to use a red-black tree for quick insert and retrieval of timelines based on a char* UUID.

- Minor bumps to authorship and small fixes

- Switched to a C++ API and example code for the sake of simplicity.

- First compiling version of the platform/hal qot abstraction. Need to test on an actual beaglebone black...

- Various bugfixes

- Converted both clock and timeline framework to use the linux/idr to dish out IDs, avoiding us needing to track a list / array.

- Committed again for backup

- More backup before bed

- Committed for backup

- Committed for backup

- Removed the DDS examples

- Add ignore to gitmodules

- Updated for backup

- Added a cycle counter and time counter implementations

- I can now open and read the clock, but it doesn't increment yet. I imagine that's because we have to implement the cycle counter, overflow checking and gettime / settime calls.

- readme.md edited online with Bitbucket

- Some progress: module code unified. "QoT Scheduler" is now available for ioctl at /dev/qot. If you run the helloworld application, you will see the creation of /dev/timelineX for each unique UUID. However, I can't seem to read the time for this posix clock. Note that you need the 80-qot.rules installed to /etc/udev/rules.d and `sudo service udev restart` to run the API / example code as a regular user.

- Added a udev rule to make the ioctl readable

- Various changes committed for backup. I learned lots about GPL licensing and symbol resolution today...

- QoT module compiles, but not tested. PTP clock next.

- WIP changes committed

- Removed target accuracy/resolution from user API

- Updates to API based on Fatima's comments

- Updated API

- Various updates to API

- Commited for backup

- Some fixes to userspace API

- Updated readme

- Various changes

- readme.md edited online with Bitbucket

- readme.md edited online with Bitbucket

- readme.md edited online with Bitbucket

- readme.md edited online with Bitbucket

- Removed verbose message

- Removed thridparty from git ignore

- Updated git modules

- Various reshuffling

- Various reshuffling

- Various changes

- Fixed submodules

- Removed all submodules

- Submodule removal

- Updated modules to drop branch spec

- Added tutorial code and updated readme

- Updated readme

- Switched to opensplice. IDL compilation and base application now working.

- Switch to OpenSplice

- Switch to OpenSplice

- Switch to OpenSplice

- Switch to OpenSplice

- Addes specific submodule branch

- Added OpenDDS submodule

- Added some code to autogenerate code from IDL messages for OpenDDS

- Various changes to directory structure

- Some changes to do inotify scan of char devices at /dev/timeline/*

- Don't try to forward messages to faulty UDS port.
When the socket couldn't be opened (e.g. in clknetsim), the file
descriptor is invalid and shouldn't be used for sending.

Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- Reset delay timer when switching to P2P delay mechanism.
When ptp4l was configured to use the auto delay mechanism and the first
pdelay request was not received in the slave or uncalibrated state, it
would not make any pdelay requests itself, because there was no delay
timer running.

Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- Merge the gPTP sync timeout branch.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add missing semicolon to enumeration.
This patch adds a semicolon forgotten in commit
5bf265e "missing: add onestep sync to missing.h"

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- phc2sys: Fix lower bound value of update_rate
Following command produces unexpected update rate.
 # phc2sys -s eth0 -q -m -O0 -R1.0842021724855044e-19
This is caused by wrong validation of phc_interval related to
significant digits of double precision and phc_interval_tp.tv_sec
overflow.

To avoid these unwanted trouble, this patch limits lower bound of
phc_rate to 1e-9.
There is no profound meaning in the lower bound value. I just think
it's enough to actual use and it doesn't cause phc_interval_tp.tv_sec
overflow in not only 64bit environment but also 32bit environment.
Thereby, messy validation of phc_interval is no longer needed.

Signed-off-by: Ken ICHIKAWA <ichikawa.ken@jp.fujitsu.com>

- missing: add onestep sync to missing.h
this patch uses grep to test whether the net_tstamp.h header has
HWTSTAMP_TX_ONESTEP_SYNC flag defined. If it doesn't then we can simply define
it with the correct value. This works because proper drivers should just report
that the value is not allowed if they don't support onestep mode. This is the
cleanest way to ensure that linuxptp will still work on kernels which have not
defined the one step flag, and also works for any distributions which backport
the feature.

Signed-off-by: Jacob Keller <jacob.e.keller@intel.com>

- Add a configuration file option for the extra sync-fup check.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add an optional extra check on sync and follow up message ordering.
Because of packet reordering that can occur in the network, in the
hardware, or in the networking stack, a follow up message can appear
to arrive in the application before the matching sync message. As this
is a normal occurrence, and the sequenceID message field ensures
proper matching, the ptp4l program accepts out of order packets.

This patch adds an additional check using the software time stamps
from the networking stack to verify that the sync message did arrive
first. This check is only useful if the sequence IDs generated by
the master might possibly be incorrect.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Announce master ambitions right away.
This patch lets a port send the first announce message one millisecond
after the port state transition, rather than waiting one announce interval.
This change is needed because it is desirable to reconfigure the time
network without delay, especially in P2P mode.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add a configuration file option for the sync receive timeout.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add a timer implementing the sync receive timeout.
This patch adds a new timer for use in 802.1AS-2011 applications. When
running as a slave in gPTP mode, the program must monitor both announce
and sync messages from the master. If either one goes missing, then we
trigger a BMC election. The sync timeout is actually reset by a valid
sync/follow up pair of messages.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Rename the timer for sending sync messages.
This patch renames the per-port timer in order to make room in the
namespace for a timer that detects a sync message input timeout.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Enforce the absolute lower limit for the announce receipt timeout.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Inhibit lost link recovery in P2P mode.
The closing and reopening of the transport when in slave only mode is not
necessary if the port is using the peer delay mechanism. In that case, the
port will discover the network error by transmitting a peer delay request.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>
Acked-by: Delio Brignoli <dbrignoli@audioscience.com>

- Do not qualify announce messages with stepsRemoved >= 255
See IEEE1588-2008 section 9.3.2.5 (d)

Signed-off-by: Delio Brignoli <dbrignoli@audioscience.com>

- Fix port numbering in clock log messages.
Add 1 to port numbers printed in the log messages to make them
consistent with messages from port.c. The port number 0 is the UDS port,
which is last in the clock->port array.

Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- Use freeifaddrs() to free data from getifaddrs().
Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- Version 1.3
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Fix bug in unlucky sync/follow up handling.
Ken Ichikawa has identified a situation in which a sync message can be
wrongly associated with a follow up after the sequence counter wraps
around.

   Port is LISTENING
   Sync (seqId 0) : ignored
   Fup  (seqId 0) : ignored
   Sync (seqId 1) : ignored
   Port becomes UNCALIBRATED here
   Fup  (seqId 1) : cached!
   Sync (seqId 2) : cached
   Fup  (seqId 2) : match
   Sync (seqId 3) : cached
   Fup  (seqId 3) : match
   ...
   Sync (seqId 65535) : cached
   Fup  (seqId 65535) : match
   Sync (seqId 0) : cached
   Fup  (seqId 0) : match
   Sync (seqId 1) : match with old Fup!!
   Fup  (seqId 1) : cached!
   Sync (seqId 2) : cached
   Fup  (seqId 2) : match

   Actually, I experienced 65500 secs offset every about 65500 secs.
   I'm thinking this is the cause.

This patch fixes the issue by changing the port code to remember one
sync or one follow up, never both. The previous ad hoc logic has been
replaced with a small state machine that handles the messages in the
proper order.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>
Reported-by: Ken ICHIKAWA <ichikawa.ken@jp.fujitsu.com>

- Be more careful when receiving clock description TLVs.
This patch adds checks to prevent buffer overruns in the TLV parsing code.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Fix check for timestamping modes
The code was checking the wrong interface's capabilities.

Signed-off-by: Andy Lutomirski <luto@amacapital.net>

- pmc: release the message cache on exit.
This makes valgrind happier by freeing any cached message buffers.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- pmc: add a command to select the target port.
This patch adds a new pmc command called "target" that lets the user
address a particular clock and port. Previously all management requests
were sent to the wild card address.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- pmc_common: introduce a variable for the target port identity.
This patch replaces the hard coded wild card target port identity with
a variable initially set to the wild card value. The intent is to allow
the caller to set specific targets.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- pmc: optional legacy zero length TLV for GET actions.
This patch makes the original behavior of sending the
TLV values for GET actions with a length of zero.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- pmc: send GET management messages according to interpretation #29.
This commit makes the GET messages have data bodies, just like the erratum
says to. It really doesn't make sense, but have to do it anyhow. We also
introduce a variable that will enable the legacy behavior of sending
empty bodies.

   http://standards.ieee.org/findstds/interps/1588-2008.html

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- pmc: make the TLV length a functional result.
This patch lets the TLV length field of GET messages come from a
function. For now, the function still results in a length of two,
but the intent is to allow different values later.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- msg: Add missing network byte order conversion.
This patch adds proper byte order processing for the target port
identity field of management messages. This bug was not previously
noticed due to the fact that our client had always set this field
to the wild card port number of 0xffff.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- ptp4l: flush old cached packets
This patch fixes a bug with time mysteriously jumping back and forth:

ptp4l[930.687]: port 1: UNCALIBRATED to SLAVE on MASTER_CLOCK_SELECTED
ptp4l[931.687]: master offset         17 s2 freq  +33014 path delay      2728
ptp4l[932.687]: master offset        -74 s2 freq  +32928 path delay      2734
ptp4l[933.687]: master offset          2 s2 freq  +32982 path delay      2734
ptp4l[934.687]: master offset         -3 s2 freq  +32977 path delay      2728
ptp4l[935.687]: master offset         17 s2 freq  +32996 path delay      2729
ptp4l[936.687]: master offset        -10 s2 freq  +32974 path delay      2729
ptp4l[937.687]: master offset         35 s2 freq  +33016 path delay      2727
ptp4l[938.686]: master offset 60001851388 s2 freq +62499999 path delay      2728
ptp4l[939.687]: master offset  -62464938 s2 freq -62431946 path delay      2728

The last follow up message arriving out of order is cached. Before the state
machine changes to UNCALIBRATED, all sync and follow up messages are discarded.
If we get into that state between a sync and follow up message, the latter is
cached. When there's no real roerdering happening, it's kept cached forever.

When we restart the master, it starts numbering the messages from zero again.
The initial synchronization doesn't take always the same amount of time, so it
can happen that we get into UNCALIBRATED a little bit faster than before,
managing to get the sync message with the sequenceId that we missed last time.
As it has the same sequenceId as the cached (old) follow up message, it's
incorrectly assumed those two belong together.

Flush the cache when changing to UNCALIBRATED. Also, do similar thing for other
cached packets.

Signed-off-by: Jiri Benc <jbenc@redhat.com>

- Become the grand master when all alone.
This patch cleans up the BMC logic to allow assuming the GM role when no
other clocks are found in the network. By allowing the "best" to be NULL,
we can let the BMC to naturally pick the local clock as GM. As an added
bonus, this also get rid of the hacky check for a lost master.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Adjust constants of PI servo according to sync interval.
Instead of using fixed constants, set them by the following formula from
the current sync to allow good performance of the servo even when the
sync interval changes in runtime and to avoid instability.

kp = min(kp_scale * sync^kp_exponent, kp_norm_max / sync)
ki = min(ki_scale * sync^ki_exponent, ki_norm_max / sync)

The scale, exponent and norm_max constants are configurable. The
defaults are chosen so there is no change to the previous default
constants of the servo with one second sync interval. The automatic
adjustment can be disabled by setting the pi_proportional_const and
pi_integral_const options to a non-zero value, but stability of the
servo is always enforced.

Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- phc2sys: Set servo sync interval.
[RC: merged servo_sync_interval signature change with earlier patch]

Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- Use a dynamic frequency estimation interval.
A slow servo (with smaller constants and lower sync rate) needs a
longer, better frequency estimation, but a higher sync rate hardly
needs any estimation at all, since it learns the frequency right away
in any case.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Let the clock servo know the expected sync interval.
This patch adds a new servo method to let the algorithm know about the
master clock's reported sync message interval. This information can be
used by the servo to adapt its synchronization parameters.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- pmc: add our very first SET command.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- pmc: add a utility function for sending a message with the SET action.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Throw a state decision event if the clock quality changes.
Management messages can cause a change in the clock quality. If this
happens, then it is time to run the Best Master Clock algorithm again.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Support configuration of the grand master settings.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- pmc: Support the grandmaster settings management request.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Support the grand master settings management query.
This patch also replaces the hard coded logic for the UTC offset and the
time property flags with clock variables. This new clock state will be
used for adjusting the grand master attributes at run time.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Introduce a non-portable management TLV for grand masters.
When running as grand master, the attributes of the local clock are not
known by the ptp4l program. Since these attributes may change over time,
for example when losing signal from GPS satellites, we need to have a
way to provide updated information at run time. This patch provides a
new TLV intended for local IPC that contains the required settings.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add a configuration option for the time source.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add a clock variable to hold the value of the time source.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add missing option to the default configuration file.
While the userDescription field is implemented in the code, the same
option is not present in the sample configuration file. This patch
fixes the issue by adding the option with an empty default value.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>
Reported-by: Rohrer Hansjoerg <hj.rohrer@mobatime.com>

- Reset announce timer when port is passive.
Whenever a port enters the passive state, it should act like a slaved
port in one respect. Incoming announce messages from the grand master
are supposed to reset the announce timer. This patch fixes the port
logic to properly maintain the passive state.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>
Reported-by: Rohrer Hansjoerg <hj.rohrer@mobatime.com>

- pmc: fix coding style by using K&R style functions.

- Clean up indented white space.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- No need to set kernel_leap twice in a row.
This patch removes a redundant initialization of the kernel_leap clock
variable. The field is already set in clock_create a few lines earlier.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add another Freescale driver into the support matrix.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Allow received GET management messages to have bodies
You'd probably expect the body of GET messages to be empty, but
interpretation #29 in
http://standards.ieee.org/findstds/interps/1588-2008.html implies
otherwise. With this change, ptp4l will respond to GETs containing
either an empty dataField or a dataField whose length matches the
managementId. If present, the contents of the dataField are ignored.

Signed-off-by: Geoff Salmon <gsalmon@se-instruments.com>

- Place clock type values in the correct positon
Signed-off-by: Dale P. Smith  <dalepsmith@gmail.com>

- ptp4l: add support for using configured_pi_f_offset servo option
This patch adds support for using the configured_pi_f_offset servo option to ptp4l.
If "pi_f_offset_const 0.0" is specified in the config file, stepping on the first
update is prevented. If any other positive value is specified, stepping on the
first update occurs when the offset is larger than the specified value.

change since v1
 - add the new option to default.cfg and gPTP.cfg

Signed-off-by: Ken ICHIKAWA <ichikawa.ken@jp.fujitsu.com>

- Add a new servo option which specifies first step threshold
Current pi servo steps clock without any condition on start.
This patch adds a new servo option "configured_pi_f_offset". The option is similar
to configured_pi_offset but only affects in the first clock update. Therefore,
if this option is set as 0.0, we can prevent clock step on start.
The new servo option can be specified from phc2sys by using -F option.

This feature is usefull when we need to restart phc2sys without system
clock jump. Restarting phc2sys is needed to change its configuration.

changes since v2:
 - manual page fix.
 - also apply max_offset along with max_f_offset in servo step1.
 - add a variable to check if first update is done.

changes since v1:(http://sourceforge.net/mailarchive/message.php?msg_id=31039874)
 - remake as a new servo option.

Signed-off-by: Ken ICHIKAWA <ichikawa.ken@jp.fujitsu.com>

- Minor documentation improvements.
Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- Add missing state setting in PI servo when date check fails.
Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- Reordered options in manual page synopsis
Options without parameters are now grouped together at the beginning of line
for better legibility.

Signed-off-by: Libor Pechacek <lpechacek@suse.cz>
Cc: Miroslav Lichvar <mlichvar@redhat.com>

- phc2sys: common code exit point for bad usage case
Removed duplicate calls to usage() by providing common exit point for the case.

Signed-off-by: Libor Pechacek <lpechacek@suse.cz>

- uClinux: Add another missing system call wrapper.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Bring the readme file up to date with the equipment donations.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- trivial: break the very long lines of the get_ functions.
The get_ranged_ and get_arg_ declarations and definitions are just a wee
bit much too long. This patch breaks the overly long lines into two.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Silence grep error during build.
Ever since upgrading to Debian 7.0, building linuxptp results in an
annoying error message. This is due to the fact that the directory
/usr/include/bits is no longer present, but our makefile expects it
to exist. This patch fixes the issue by telling grep not to complain
about missing files.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- config: Apply more strict input validation to almost all config file options
Signed-off-by: Ken ICHIKAWA <ichikawa.ken@jp.fujitsu.com>

- Don't return bogus clock id
phc_open() can open any device and return clkid even if the device is not phc
for example /dev/kvm and so on.
As a result, phc2sys keeps running with reading bogus clock as below:
 # phc2sys -s /dev/kvm -O 0 -q -m
 phc2sys[687019.699]: failed to read clock: Invalid argument
 phc2sys[687020.699]: failed to read clock: Invalid argument
 phc2sys[687021.699]: failed to read clock: Invalid argument
 phc2sys[687022.699]: failed to read clock: Invalid argument
 ...

This patch fixes that problem.

Signed-off-by: Ken ICHIKAWA <ichikawa.ken@jp.fujitsu.com>

- ptp4l and phc2sys: Get argument values with strict error checking
Signed-off-by: Ken ICHIKAWA <ichikawa.ken@jp.fujitsu.com>

- util: Add common procedures to get argument values for ptp4l and phc2sys
Signed-off-by: Ken ICHIKAWA <ichikawa.ken@jp.fujitsu.com>

- config: Apply config value validation to logging_level option
Signed-off-by: Ken ICHIKAWA <ichikawa.ken@jp.fujitsu.com>

- Add support for more strict config value validation
This patch adds functions to get int, uint, double value from string
with error checking and range specification.
These functions don't allow overflow and outside of the range values.

In addition, it adds parser_result cases "MALFORMED" and "OUT_OF_RANGE" to make
reason of parse error more clear.

Signed-off-by: Ken ICHIKAWA <ichikawa.ken@jp.fujitsu.com>

- phc2sys: Require either -O or -w on command line
The default zero offset can lead to misalignment between system clocks or wrong
time to be broadcast to the domain.  Therefore we require setting offset upon
invocation.

Signed-off-by: Libor Pechacek <lpechacek@suse.cz>

- Document PTP time scale usage and provide examples
Signed-off-by: Libor Pechacek <lpechacek@suse.cz>

- Fix parsing of fault_badpeernet_interval option
Signed-off-by: Delio Brignoli <dbrignoli@audioscience.com>

- ptp4l: Reset path delay when new master is selected.
When a new master is selected, drop the old path delay and don't
calculate the offset until the delay is measured again with the new
master.

Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- phc2sys: update usage/error reporting
This patch updates phc2sys usage reporting to give a slightly better indication
of why the program was unable to run.

Signed-off-by: Jacob Keller <jacob.e.keller@intel.com>

- phc2sys: update open_clock and deprecate '-i' option
This patch modifies phc2sys to enable the use of interface names in clock_open
rather than having to do that by hand. This enables cleaner use of the -s and -c
options as they can accept interface names. This also enables the user to set
the slave clock by network interface as well.

-v2-
* fix clock_open as it used device instead of phc_device in the final call to phc_open

Signed-off-by: Jacob Keller <jacob.e.keller@intel.com>

- phc2sys: Use nanosleep instead of usleep.
[RC: use CLOCK_MONOTONIC as suggested on the list.]

Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- fix the check for supported timestamping modes
Fix the check for supported timestamping modes. The device needs
to support all the required modes, not just any subset of them.

Signed-off-by: Jiri Bohac <jbohac@suse.cz>

- fix misleading pr_err on poll timeout
If poll() times out, don't print a misleading errno, say that a timeout
occured.

Signed-off-by: Jiri Bohac <jbohac@suse.cz>

- phc2sys: Change update rate parameter to floating-point.
This allows update intervals longer than 1 second.

Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- phc2sys: Use currentUtcOffset only with PTP timescale.
Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- ptp4l: Allow P, I constants over 1.0.
With sub-second sync intervals, it may be useful to set P and I to
values over 1.0.

Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- Version 1.2
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- phc2sys: enable PPS output from PHC
PPS output from a PHC has to be enabled by PTP_ENABLE_PPS ioctl. Call
the ioctl when both PHC device and PPS device are specified and PPS is
supported by the PHC.

Signed-off-by: Jiri Benc <jbenc@redhat.com>

- Simplify tests on configuration ranges.
This patch simplifies some expressions which validate that configuration
variables are within the allowed ranges.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Replace spaces with tabs in configs.
Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- Add option to set maximum frequency adjustment.
The option sets an additional limit to the hardware limit. It's disabled
if set to zero. The default is 900000000 ppb.

Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- Let kernel synchronize RTC to system clock.
Reset the STA_UNSYNC flag and maxerror value with every frequency update
to let the kernel synchronize the RTC to the system clock (11 minute mode).

Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- clockadj: Remove clkid parameter from set_leap function.
Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- phc2sys: Add option to set domain number.
Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- phc2sys: Use phc_open().
Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- phc2sys: Don't try PTP_SYS_OFFSET with system clock as source.
Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- phc2sys: Read maximum frequency adjustment.
Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- clockadj: remove useless clockid parameter.
The clockid parameter to the function to get the system clock's maximum
adjustment is redundant, so let us just remove it.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- clockadj: make Doxygen comment by using two stars.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- ptp4l: Use poll() instead of a try-again loop
This patch modifies sk_receive in order to use poll() on POLLERR instead of the
tryagain loop as this resolves issues with drivers who do not return timestamps
quickly enough. It also resolves the issue of wasting time repeating every
microsecond. It lets the kernel sleep our application until the data or timeout arrives.

This change also replaces the old tx_timestamp_retries config value with
tx_timestamp_timeout specified in milliseconds (the smallest length of time poll
accepts). This does have the side effect of increasing the minimum delay before
missing a timestamp by up to 1ms, but the poll should return sooner in the
normal case where a packet timestamp was not dropped.

This change vastly improves some devices and cleans the code up by simplifying a
race condition window due to drivers returning tx timestamp on the error queue.

[ RC - removed the unused 'try_again' variable. ]

Signed-off-by: Jacob Keller <jacob.e.keller@intel.com>

- Apply utc offset correction even when free running.
When using software time stamping and a free running clock, the
statistics appear to be off by the TAI offset. This patch fixes the
issue by setting the internal UTC timescale flag in such cases.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Fix compiler warnings with -O2.
Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- ptp4l: Read system clock maximum frequency adjustment.
Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- ptp4l: Set clock frequency on start.
Due to a bug in older kernels, frequency reading may silently fail and
return 0. Set the frequency to the read value to make sure the servo
has the actual frequency of the clock.

Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- Let a slaved port update the time properties on every announce message.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add a clock method to update the time properties data set.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Distinguish between ignored and malformed packets
When there is a peer speaking PTPv1 in the network we want to silently ignore
the packets instead of flooding system log with error messages.  At the same
time we still want to report malformed packets.  For that we reuse standard
error numbers and do more fine-grained error reporting in packet processing
routines.

Signed-off-by: Libor Pechacek <lpechacek@suse.cz>

- phc2sys: add help messages for -l, -m and -q
This patch adds help messages for -l, -m and -q options.
Also it swaps -h for -v because ptp4l's help is this order.

Signed-off-by: Ken ICHIKAWA <ichikawa.ken@jp.fujitsu.com>

- Enable LOG_MIN_PDELAY_REQ_INTERVAL management request
Why don't you enable LOG_MIN_PDELAY_REQ_INTERVAL management request?

- hwstamp_ctl: explain ERANGE error better
ERANGE is used by the kernel to indicate the hardware does not support the
requested time stamping mode. Explain this error to the user.

Signed-off-by: Jiri Benc <jbenc@redhat.com>

- Merge branch 'mlichvar_leap'
Fixed up trivial conflict in the makefile.

Conflicts:
	makefile

- Add options to not apply leap seconds in kernel.
Add kernel_leap option for ptp4l and -x option for phc2sys to disable
setting of the STA_INS/STA_DEL bit to slowly correct the one-second
offset by servo.

Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- ptp4l: Handle leap seconds.
Extend the clock_utc_correct function to handle leap seconds announced
in the time properties data set. With software time stamping, it sets the
STA_INS/STA_DEL bit for the system clock. Clock updates are suspended
in the last second of the day.

Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- Add missing conversions from tmv_t to nanoseconds.
Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- phc2sys: Handle leap seconds.
Update the currentUtcOffset and leap61/59 values at one minute
interval. When a leap second is detected, set the STA_INS/STA_DEL
bit for the system clock.

Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- Move clock_adjtime wrapping to clockadj.c.
Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- phc2sys: Update sync offset periodically with -w.
Modify the pmc to allow non-blocking operation. Run it on each clock
update to have the sync offset updated from currentUtcOffset with every
other call.

Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- Add support for FT_BAD_PEER_NETWORK
Handle reception of >=3 sequential multiple pdelay responses from
distinct peers as a fault of type FT_BAD_PEER_NETWORK.

Signed-off-by: Delio Brignoli <dbrignoli@audioscience.com>

- Add support for multiple fault types
Signed-off-by: Delio Brignoli <dbrignoli@audioscience.com>

- Rename set_tmo() to set_tmo_log(), add set_tmo_lin()
Signed-off-by: Delio Brignoli <dbrignoli@audioscience.com>

- Explicitly detect and handle changes of the peer's port id by resetting asCapable and the port's nrate
This patch also changes port_capable() to reset the port's nrate every time asCapable changes
from true to false.

Signed-off-by: Delio Brignoli <dbrignoli@audioscience.com>

- Log changes to asCapable
Signed-off-by: Delio Brignoli <dbrignoli@audioscience.com>

- Free peer delay responses and followup messages when sending a new peer delay request
If messages are not freed, it is possible (with purposely crafted traffic) to trigger
a peer delay calculation which will use message's data from the previous round.

Signed-off-by: Delio Brignoli <dbrignoli@audioscience.com>

- Implement neighborPropDelayThresh check in port_capable()
Signed-off-by: Delio Brignoli <dbrignoli@audioscience.com>

- pmc: avoid printing invalid data from empty TLVs
Also adds additional null-check to bin2str to avoid crashing on empty
messages.

Signed-off-by: Geoff Salmon <gsalmon@se-instruments.com>

- Align the message buffer to eight bytes.
The 'struct ptp_message" includes a 64 bit integer field, ts.pdu.sec,
and this must be aligned to an eight byte boundary for armv5 machines.
Although the compiler puts the field at the right offset, we spoil this
by packing the struct with 20 bytes of head room. This patch fixes the
issue by realigning the message buffer.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add support for the log peer delay interval management request.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add support for the delay mechanism management request.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add support for the version number management request.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add support for the log sync interval management request.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add support for the announce receipt timeout management request.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add support for the log announce interval management request.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add support for the timescale management request.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add support for the traceability management request.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add support for the clock_accuracy management request.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add support for the slave_only management request.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add support for the domain management request.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add support for the priority2 management request.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add support for the priority1 management request.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add an all purpose, single byte management TLV.
Many of the single field management messages have just two bytes, one for
the data value and one for padding. This patch adds a structure that can
be used for all of these management IDs.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- pmc: get CLOCK_DESCRIPTION and USER_DESCRIPTION
Signed-off-by: Geoff Salmon <gsalmon@se-instruments.com>

- support GET CLOCK_DESCRIPTION and USER_DESCRIPTION mgmt messages
Signed-off-by: Geoff Salmon <gsalmon@se-instruments.com>

- adds CLOCK_DESCRIPTION struct
Modifies existing structs changing Octet *foo -> Octet foo[0] and
marks them PACKED so the message buffer can be cast to the structs.

Signed-off-by: Geoff Salmon <gsalmon@se-instruments.com>

- support TLVs with flexible size
These flexible TLVs don't need to be represented as a single packed
struct directly in the message buffer. Instead a separate struct
contains pointers to the individual parts of the TLV in the message
buffer. The flexible TLV can only be the last TLV in a message.

Signed-off-by: Geoff Salmon <gsalmon@se-instruments.com>

- Version 1.1
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- avoid hton conversion on empty management msgs
Signed-off-by: Geoff Salmon <gsalmon@se-instruments.com>

- Check shift used in freq_est and stats max_count calculation.
Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- Change stats max_count variables to unsigned.
Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- set length of ptp text defaults
Signed-off-by: Geoff Salmon <gsalmon@se-instruments.com>

- Reduce the arguments to clock_create.
New clock options should go into 'struct default_ds' so that we can avoid
growing clock_create indefinitely.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add summary statistics.
Add new options to ptp4l and phc2sys to print summary statistics of the
clock instead of the individual samples.

[ RC - Fix () function prototype with (void).
     - Use unsigned for sample counter.
     - Fix over-zealous line breaks. ]

Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Make the counters for the frequency and rate estimators unsigned.
These are simple 'up' counters.
There is no need for negative values here.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- match pmc_send_get_action's definition and declaration
Signed-off-by: Geoff Salmon <gsalmon@se-instruments.com>

- phc2sys: Print clock reading delay.
If the delay is known, print it together with the offset and frequency.
Remove the time stamp from the output to fit it into 80 chars.

Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- Change label of frequency offset.
Change the label of the frequency offset in the clock messages printed
by ptp4l and phc2sys from "adj" to "freq" to indicate it's a frequency
value.

Also, modify clock_no_adjust() to print the frequency offset instead of
the ratio and use PRId64 instead of lld to print int64_t values.

Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- Fix initialization of frequency estimation interval.
The clock_sync_interval() function is called when logSyncInterval
changes from zero. Call it also when the clock is created to have
fest.max_count set accordingly to freq_est_interval even with zero
logSyncInterval.

Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- Update port state strings.
Update the order of the strings to reflect the changes made by commit
f530ae93331f878afdeb611bffce85b99f6636fb.

Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- Remove trailing whitespaces.
Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- Update comment in pi.c.
This was missed in commit d8cb9be46a225a159168581531d401fa2eff8c68.

Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- Check sample time stamps in drift calculation.
Before calculating the clock drift in the PI servo, make sure
the first sample is older than the second sample to avoid getting
invalid drift or NaN.

Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- Print messages with level below LOG_NOTICE to stderr.
Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- phc2sys: Use printing facility.
Use pr_* functions to print messages and add -m, -q, -l options to allow
configuration of the printing level and where should be the messages sent.

Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- phc2sys: Open PPS device sooner.
Possible error messages should be printed before waiting on ptp4l.

Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- pmc: Allow commands on command line.
Add a batch mode, where the commands are taken from the command line
instead of the standard input.

Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- Move Unix domain sockets to /var/run.
According to FHS, /var/run is the right place for them.

Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- phc2sys: Add option to set step threshold.
Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- Send peer delay requests continuously in P2P mode.
When a port makes a transition from one state to another, it resets all of
the message timers. While this is the correct behavior for E2E mode, the
P2P mode requires sending peer delay requests most of the time.

Even though all the other timer logic is identical, still making an
exception for P2P mode would make the code even harder to follow. So this
patch introduces two nearly identical helper functions to handle timer
reprogramming during a state transition.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- send errors if mgmt tlv length doesn't match action
It's especially important to check that SET messages aren't empty.

Signed-off-by: Geoff Salmon <gsalmon@se-instruments.com>

- send NOT_SUPPORTED errors for all unhandled, known management IDs
Now that there are clock/port_management_set functions, the IDs that
GETs are handled for, like DEFUALT_DATA_SET, still need to be in the
case for sending NOT_SUPPORTED errors.

Signed-off-by: Geoff Salmon <gsalmon@se-instruments.com>

- adds stub clock/port_management_set functions
Signed-off-by: Geoff Salmon <gsalmon@se-instruments.com>

- send UNKNOWN_ID error for unknown management TLVs
Signed-off-by: Geoff Salmon <gsalmon@se-instruments.com>

- factors out functions for sending mgmt errors from clock and port
Adds port_management_send_error and clock_management_send_error to
avoid repeatedly checking the result of port_managment_send_error and
calling pr_err if it failed. Future patches send more mgmt errors so
this will avoid repeated code.

Signed-off-by: Geoff Salmon <gsalmon@se-instruments.com>

- fixes typo port_managment_error -> port_management_error
Signed-off-by: Geoff Salmon <gsalmon@se-instruments.com>

- Add a coding style document.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Hide the grandmaster port state.
There really is no such state, but there probably should have been one.
In any case, we do have one just to make the code simpler, but this should
not appear in the management responses. This patch fixes the issue by
covering over our tracks before sending a response.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- config clock description
Signed-off-by: Geoff Salmon <gsalmon@se-instruments.com>

- adds clock description
Adds struct containing clock description info that will be needed for
USER_DESCRIPTION and CLOCK_DESCRIPTION management messages.

Signed-off-by: Geoff Salmon <gsalmon@se-instruments.com>

- adds static_ptp_text and functions for setting PTPText and static_ptp_text
static_ptp_text is like a PTPText that includes space for the text
which makes it more convenient for ptp texts stored in the clock.

Signed-off-by: Geoff Salmon <gsalmon@se-instruments.com>

- phc2sys: include PTP management client.
Add a new option to wait for ptp4l to be in a synchronized state.
Periodically check PORT_DATA_SET and wait until there is a port in
SLAVE, MASTER or GRAND_MASTER state. Also, set the default
synchronization offset according to the currentUtcOffset value from
TIME_PROPERTIES_DATA_SET and the direction of the clock synchronization.

Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- Move some pmc code to separate file.
Move some code which can be shared between PTP management clients to
a new file.

Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- Set program name for print().
The printing facility is used by different programs, allow to set the
program name which prefixes the messages written to stdout.

Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- Merge the branch with the 'asCapable' support.
Conflicts:
	port.c

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Stop handling file descriptor events after a port reset.
If the port resets itself after detecting a fault, then the polling events
for that port are no longer valid. This patch fixes a latent bug that
would appear if a fault and another event were to happen simultaneously.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Make the fault reset interval a per-port configuration parameter.
A timeout of 15 seconds is not always acceptable, make it configurable.

By popular consensus, instead of using a linear number of seconds, use
the 2^N format for the time interval, just like the other intervals in
the PTP data sets. In addition to numeric values, let the configuration
file support 'ASAP' to have the fault reset immediately.

[RC - moved the handling of special case tmo=0 and added a break out
      of the fd event loop in case the fds have been closed.
    - changed the linear seconds option to log second instead.
    - changed the commit message to reflect the final version. ]

Signed-off-by: Delio Brignoli <dbrignoli@audioscience.com>
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Fix fault-clearing timer being erroneously re-armed
Arm the fault-clearing timer only when an event causes a port to change state
to PS_FAULTY. Previously, if poll() returned because of an fd event other than
the fault-clearing timeout, the fault clearing timer would re-arm for
each port in PS_FAULTY state.

Signed-off-by: Delio Brignoli <dbrignoli@audioscience.com>

- Let a port become 'capable' according to 802.1AS.
This patch implements the capable flag as follows.

1. After calculating the neighbor rate, we are capable.
2. If we miss too many responses, we are incapable.
3. If we get multiple responses, we throw a fault,
   and so we are also incapable.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add another Intel driver into the driver support matrix.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add a default of 'incapable' for 802.1AS mode.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add port logic for the 'capable' flag from 802.1AS.
This commit only provides helper functions that will implement the effect
of a port being not capable. We let the port be always 'capable' for now,
until we actually have added the details of that flag.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Introduce a test for running in 802.1AS mode.
We use the follow_up_info to control behavior that is specific to the
802.1AS standard. In several instances, that standard goes against the
1588 standard or requires new run time logic that exceeds what can be
reasonably described as a 1588 profile.

Since we will need a few more run time exceptions in order to support
802.1AS, we introduce a helper function to identify this case, rather
than hard coding a test for follow_up_info, in order to be more clear
about it.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Start P2P messages right away when listening after initializing.
Because of an oversight in the event code, a port will not send peer delay
request messages while in the initial listening state. This patch fixes
the issue by expanding this special, initial case.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Check that TLV length is correct when receiving TLVs.
The function, tlv_post_recv, and the functions it calls don't check
the length of the tlv before flipping the byte order of fields. An
attacker (or a really buggy client) can craft a message causing the
byte order of data outside the received message to be flipped.

None of the supported tlvs are large enough to flip bytes outside the
ptp_message struct, which could corrupt the heap. However, it's easy
to mess up the message's refcnt field, leading to memory leaks.

The fix is to check that the tlv length is what is expected when
receiving, and tlv_post_recv needs to return an int to signal when a
tlv is invalid.

Signed-off-by: Geoff Salmon <gsalmon@se-instruments.com>

- Add missing documentation for msg_post_recv.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- phc2sys: allow PPS loop only with system clock.
The PPS time stamps are always made by the system clock, don't allow
running the PPS loop with other clocks.

Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- phc2sys: read PHC with each PPS sample.
In the PPS loop, instead of setting the system clock from the PHC only
once on start, read PHC with each PPS sample and use the time stamp to
get the whole number of seconds in the offset. This will prevent phc2sys
from losing track of the system clock.

Also, check if the PPS is synchronized to the PHC.

Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- phc2sys: remove unnecessary clock step with non-PPS loops.
With non-PPS loops let the servo make the inital correction. Move the
code to the PPS loop and change it to use the sample filtering to reduce
the error in the initial correction.

Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- phc2sys: don't zero clock frequency on start.
Instead of always starting at zero frequency offset, read the currently
stored value on start and pass it to the servo. As the read may silently
fail and return zero, set the clock frequency back to the read value to
make sure it's always equal to the actual frequency of the clock.

Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- phc2sys: move phc loop to its own function.
Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- phc2sys: use servo code from ptp4l.
Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- Remove unnecessary states in PI servo.
Step the clock as soon as the second sample is collected.

Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- Modify PI servo to set frequency when jumping.
Similarly to the servo in phc2sys, when clock is stepped, set
immediately also its frequency. This significantly improves the initial
convergence with large frequency offsets.

Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- Modify servo_sample() to accept integer values.
Current date stored in nanoseconds doesn't fit in 64-bit double format.
Keep the offset and the time stamp in integer nanoseconds.

Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- Fix initial drift calculation in PI servo.
Convert the calculated drift to ppb and also clamp it.

Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- udp6: implement getting physical and protocol addrs

- udp: implement getting physical and protocol addrs

- add sk_interface_addr for getting an interface's IP

- raw: implement getting physical and protocol addrs

- transport: adds interface for getting type, and physical/protocol address
Needed for CLOCK_DESCRIPTION management TLV.

- Make enum transport_type match the networkProtocol enumeration
This means no conversion is necessary between the transport_type and
the networkProtocol field of the PortAddress struct. Not currently an
issue, but will be needed for implementing the CLOCK_DESCRIPTION
management TLV.

- Avoid calling msg_pre_send/post_recv unless actually forwarding a message

- pmc: prefer exact matches for command names
Previously if a command's full name was a prefix of another command
then parse_id would return AMBIGUOUS_ID. This was a problem for the
TIME and various TIME_* messages.

- Add the SF download URL into the readme file.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- phc2sys: check that specified interface has a PHC.

- Fix -Wformat warnings.

- Update twoStepFlag description in man page.

- Update the readme with another Linux PHC driver.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Version 1.0
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add asymmetry correction.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>
Acked-by: Jacob Keller <Jacob.e.keller@intel.com>

- Add command line options to print the software version.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add utility functions to get the software version string.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add a compile time version string definition.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add a shell script to generate the version string.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- pmc: show the flags from the default data set.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- pmc: fix missing include.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- pmc: reduce the message time out waiting for replies.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- pmc: show the port state as a string.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Distinguish between get and set management requests.
The code previously treated all supported request as 'get' actions and
ignored the actual action field in the message. This commit makes the
code look at the action field when processing the requests.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- pmc: correct the integer format string wrt decimal verses hexadecimal.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- pmc: support getting the port data set.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Respond to the port data set management query.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Maintain the peer mean path delay field of the port data set.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Convert the port data set TLV to and from host byte order.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- pmc: support getting the time properties data set.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Respond to the time properties data set management query.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Convert the time properties data set TLV to and from host byte order.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Use the management message memory layout for the timePropertiesDS.
Reforming the data structure in this way will greatly simplify the
implementation of the management message for this data set.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- pmc: remove trailing spaces from format strings.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- pmc: support getting the parent data set.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Respond to the parent data set management query.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Convert the parent data set TLV to and from host byte order.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- pmc: support getting the default data set.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Respond to the default data set management query.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Convert the default data set TLV to and from host byte order.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Use the management message memory layout for the parentDS.
Reforming the data structure in this way will greatly simplify the
implementation of the management message for this data set.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Use the management message memory layout for the defaultDS.
Reforming the data structure in this way will greatly simplify the
implementation of the management message for this data set.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Implement the NULL management message.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Ignore management response messages.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- udp: work around phyter quirk.
The phyter offers one step sync transmission, but it alters the UDP
checksum by changing the last two bytes, after the PTP payload. While this
is only standardized for IPv6, we will go with it for IPv4 as well, since
the phyter is the only hardware out there.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Support one step sync operation.
The Linux kernel supports a hardware time stamping mode that allows
sending a one step sync message. This commit adds support for this mode
by expanding the time stamp type enumeration. In order to enable this
mode, the configuration must specify both hardware time stamping and set
the twoStepFlag to false.

We still do not support the one step peer delay request mechanism since
there is neither kernel nor hardware support for it at this time.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Enumerate the event codes for the transport layer transmission methods.
We add a new event code that indicates a one step event message.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- phc2sys: use the system offset method if available
We use the PTP_SYS_OFFSET ioctl method in preference to doing
clock_gettime from user space, the assumption being that the kernel space
measurement will always be more accurate.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add a method to estimate the PHC to system offset using the new ioctl.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- readme: correct blackfin entry in the driver support matrix.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Document the udp6_scope configuration option in the man page.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- udp6: add an option to set the multicast scope.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- udp6: set the scope id when sending to link local addresses.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- udp6: remember the interface index.
In order to set the scope id, we will need to know the interface index
before sending, in some cases.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Initialize the socket address structure to zero.
It is safer and more correct to clear the addresses before use. The IPv6
fields flowinfo and scope_id in particular should not be set to random
values from the stack.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Tell about the make install target in the readme file.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Update the readme for Linux kernel version 3.7.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Fixup the Doxygen comment for the sk_ts_info struct.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Fixup label defined but not used warning
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- ptp4l: use sk_get_ts_info to check timestamping mode
this patch uses the new sk_get_ts_info data to check whether the selected
timestamping mode is supported on all ports. This check is not run if the port
data isn't valid, so it will only fail if the IOCTL is supported. This allows
for support of old kernels that do not support the IOCTL to still work as expected.

Signed-off-by: Jacob Keller <jacob.e.keller@intel.com>

- sk: change sk_interface_phc to sk_get_ts_info
this patch changes sk_interface_phc to sk_get_ts_info, by allowing the function
to store all the data returned by Ethtool's get_ts_info IOCTL in a struct. A new
struct "sk_ts_info" contains the same data as well as a field for specifying the
structure as valid (in order to support old kernels without the IOCTL). The
valid field should be set only when the IOCTL successfully populates the fields.

A follow-on patch will add new functionality possible because of these
changes. This patch only updates the programs which use the call to perform the
minimum they already do, using the new interface.

Signed-off-by: Jacob Keller <jacob.e.keller@intel.com>

- ptp4l: make config parser strict.
- fail on unknown options, bad values and other syntax errors
- fail with more than MAX_PORTS ports
- remove implicit global section
- add error messages

Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- ptp4l: ignore empty lines and comments in config.
Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- phc2sys: add option to set slave-master time offset.
Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- ptp4l: increase default tx_timestamp_retries to 100.
It seems with some cards the current default of 2 is too small, increase
the number to prevent users from having to investigate the EAGAIN error
messages.

Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- makefile: add install target.
Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- Add .gitignore.
Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- Add an acknowledgment in the readme for audioscience.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- phc2sys: update help message.
Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- phc2sys: add selecting clock by name.
This allows synchronization of a PHC to the system clock.

Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- Add man pages.
Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- Remove useless option selecting layer 2 time stamping.
Now that the code automatically falls back to transport-specific time
stamping, this option is no longer needed.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>
Acked-by: Jacob Keller <jacob.e.keller@intel.com>
Tested-by: Jiri Benc <jbenc@redhat.com>

- Try two different HWTSTAMP options.
Start with the most general HWTSTAMP option. If that fails, fall back
to the option that best fits the interface's transport.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>
Acked-by: Jacob Keller <jacob.e.keller@intel.com>
Tested-by: Jiri Benc <jbenc@redhat.com>

- Pass transport type to time stamping initialization function.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>
Acked-by: Jacob Keller <jacob.e.keller@intel.com>
Tested-by: Jiri Benc <jbenc@redhat.com>

- Fix errors found by Coverity Static Analysis.
Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- Explicitly set default values for all configurable options.
Make it easier to find out the compiled-in defaults in the code
and set all options in default.cfg.

Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- Don't try calling SIOCETHTOOL on UDS.
Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- Remove obsolete links from the readme.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Allow zero as a configured value for pi_offset_const.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- ptp4l: update the configuration files
this patch updates the default.cfg and gPTP.cfg to show the new defaults

Signed-off-by: Jacob Keller <jacob.e.keller@intel.com>

- ptp4l: add maximum offset to pi servo
this patch modifies the pi servo to add a configurable max offset (default
infinity). When ever the detected offset is larger than this value, the clock
will jump and reset the servo state. The value of this feature is for decreasing
time to stabalize when clock is off by a large ammount during late running. This
can occur when the upstream master changes, or when the clock is reset due to
outside forces. The method used to reset clock is simply to reset the pi servo
to the unlocked state.

Signed-off-by: Jacob Keller <jacob.e.keller@intel.com>

- ptp4l: add servo selection to the configuration file
this patch adds the servo selection to the configuration file

Signed-off-by: Jacob Keller <jacob.e.keller@intel.com>

- ptp4l: modify clock_create to take servo as argument
this patch modifies the clock_create to take the servo type as an argument

Signed-off-by: Jacob Keller <jacob.e.keller@intel.com>

- ptp4l: modify servo setup to take an enum rather than string
passing a string as the servo type seems ugly when there are only a few
choices. This patch modifies the servo_create to take an enum instead.

Signed-off-by: Jacob Keller <jacob.e.keller@intel.com>

- phc2sys: add sk.o to phc2sys for sk_interface_phc
This patch adds support for sk_interface_phc to lookup the /dev/ptpX device
using ethtool's get_ts_info ioctl

-v2-
* set ethdev to null initially
* rebased on top of phc2sys changes

-v3-
* fixed typo regarding slash

Signed-off-by: Jacob Keller <jacob.e.keller@intel.com>

- phc2sys: refactor and rationalize the output messages.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- phc2sys: disentangle the PPS loop from the read_phc loop.
The main loop has become a bit messy, mixing two completely disjunct code
paths. This patch separates the two use cases for the sake of clarity.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- phc2sys: remove useless option.
The 'rdelay' option has been obsoleted by the newer read_phc() logic.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- phc2sys: remove unused variable.
The 'src' parameter to the do_servo() function is never used, so
remove it.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Discover and utilize the initial clock frequency adjustment.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Warn if a slave only node is selected by the bmc.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Merge branch 'as2'

- phc2sys: fix operation without -s option.
Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- Do not expect or open a /dev/ptpX device when in free_running mode
Signed-off-by: Delio Brignoli <dbrignoli@audioscience.com>

- Introduce the time status management message.
This non-portable, implementation specific message is designed to inform
external programs about the relationship between the local clock and the
remote master clock.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- trivial: fix a spelling error in a comment.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Introduce a helper function to convert from tmv to nanoseconds.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- bugfix: use a sensible test to detect a new master.
The code to detect a new master used pointer equality using a stale
pointer within the clock instance. Instead, the clock needs to remember
the identity of the foreign master in order to correctly detect a
change of master.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Use clock_adjtime from glibc if available.
Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- bugfix: correct wrong type for the cumulativeScaledRateOffset field.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Guard against divide by zero.
If a buggy driver or hardware delivers bogus time stamps, then we might
crash with a divide by zero exception.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Introduce a macro for the constant 2^41.
We are going to need this more than once for working with the
cumulativeScaledRateOffset fields.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add an implementation specific management code.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Be tolerant of wrong a OUI in the follow up information.
Certain AVB bridges send their own OUI instead of the offical one.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Remove unnecessary wait state from frequency estimator.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Calculate the master/local rate ratio in two ways.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Extract the follow up info and pass it to the clock.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add a clock method to receive the follow up information TLV.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Provide the clock with the estimated neighbor rate ratio.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Estimate the neighbor rate ratio from a slaved port.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Give the frequency estimation interval to the port as well.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Differentiate between the E2E and P2P delay timeout intervals.
We have one timer used for both delay request mechanisms, and we ought
to set the message interval accordingly.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Adjust the peer delay request message interval field.
IEEE 802.1AS-2011 specifies using the current logMinPdelayReqInterval
for this field.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Reset the master/local frequency estimator when changing masters.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Invert the frequency ratio estimation.
In 802.1AS-2011 the ratio is defined as master/local, so we should follow
suit.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Rework the frequency ratio estimator to use the tmv functions.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Provide a conversion function from tmv to double.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Correct a comment about the frequency ratio estimation.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Fix copy-pasto in config file parsing of the logging level.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- phc2sys: add options for number of PHC readings and update rate

- phc2sys: improve servo start up

- Add an acknowledgment in the readme of the hardware donors.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Bring the readme up to date.
- all of the original goals have been implemented
- add gPTP to the list of supported features

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Improve stability of PHC reading in phc2sys.
Make five PHC readings in sequence and pick the quickest one.

Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- Add non-PPS mode to phc2sys.
As some PHC hardware/drivers don't provide PPS, it may be useful to keep the
system clock synchronized to PHC via clock_gettime(). While the precision
of the clock is only in microsecond range, the error seems to be quite stable.

The -d parameter now can be omitted if -s is provided.

Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- Revert "Add non-PPS mode to phc2sys."
This reverts commit 998093b6f7dbf1fa633ecd61c757da8b52a7ffae.
Will apply this patch again, with the correct author info.

- Revert "Improve stability of PHC reading in phc2sys."
This reverts commit f08d264e1d23d83554352e097ec17535d06f7725.
Will apply this patch again, with the correct author info.

- Improve stability of PHC reading in phc2sys.
Make five PHC readings in sequence and pick the quickest one.

Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- Add non-PPS mode to phc2sys.
As some PHC hardware/drivers don't provide PPS, it may be useful to keep the
system clock synchronized to PHC via clock_gettime(). While the precision
of the clock is only in microsecond range, the error seems to be quite stable.

The -d parameter now can be omitted if -s is provided.

Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- pmc: add a simple interactive help command.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Always set the clock class to 255 when slave only mode is configured.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Drop the reference to best when freeing the foreign masters.
This fixes the following issue reported by valgrind, which occurs
after a port disable/initialize subsequent to having entered slave
mode.

==10651== Invalid read of size 4
==10651==    at 0x804E6E2: fc_clear (port.c:175)
==10651==    by 0x805132F: port_event (port.c:1352)
==10651==    by 0x804B383: clock_poll (clock.c:597)
==10651==    by 0x80498AE: main (ptp4l.c:278)
==10651==  Address 0x41cba60 is 16 bytes inside a block of size 60 free'd
==10651==    at 0x4023B6A: free (vg_replace_malloc.c:366)
==10651==    by 0x804EB09: free_foreign_masters (port.c:287)
==10651==    by 0x804FB14: port_disable (port.c:722)
==10651==    by 0x8051228: port_dispatch (port.c:1298)
==10651==    by 0x804B3C6: clock_poll (clock.c:602)
==10651==    by 0x80498AE: main (ptp4l.c:278)

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Configure logging facility sooner.
Error messages from initialization were missing.

Signed-off-by: Miroslav Lichvar <mlichvar@redhat.com>

- Release all resources when quitting the main loop.
This will allow running under valgrind to detect memory leaks
and the like.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Make the clock release method a public function.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Provide a method to release the message cache.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Break out of the main loop on Control-C.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- uds: fix bug in file descriptor array opening.
By not touching the event descriptor, uds is leaving the value as zero,
resulting in polling on stdin.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Correct the TAI-UTC offset when it is reasonable to do so.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Warn if the master's time properties are suspicious.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Update the hard coded TAI-UTC offset.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Use zero for the UDS port number.
Using 0xffff looks dumb in the log and in the pmc output.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Use the ingress port identity for clock management replies.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Provide a method to obtain a port's identity.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Clean up the tlv and pmc include directives.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Move shared layer 2 global declarations into an appropriate header file.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Move shared PI global variable into the proper module.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- pmc: look for POLLHUP in order to work with piped input and redirection.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- pmc: add command line options to control messaging.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- pmc: provide a reasonable default path when using UDS.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- pmc: optionally use the UDS transport.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Create one special UDS port per clock.
This port is handled a bit differently than the others. Its only purpose
is to accept management messages from the local machine.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Introduce transport over UNIX domain sockets.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- pmc: show the fields of the current data set responses.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Respond to the current data set management query.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Convert the current data set TLV to and from host byte order.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Maintain the current data set.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- pmc: support getting the current data set.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- pmc: make the interactive output a bit clearer.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Drop stale delay requests after the clock jumps in time.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Throw an error if SIOCETHTOOL returns a bad PHC index.
If the kernel supports the ioctl, but the driver does not (like igb in
kernel version 3.5), then ptp4l will incorrectly choose the system clock.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Estimate the local/master frequency ratio when free running.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Let a slaved port report the sync interval.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Provide a method to report the sync interval to the clock.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add an option to dial the frequency estimation interval.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add an option to configure a free running local clock.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Restore reading of the configuration file on stdin.
This feature is was dropped by accident in commit e1d3dc56.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Remove obsolete text from command line usage.
Now the command line options are always global and not per-interface.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Free the clock's servo and moving average on destroy.
The destroy method is supposed to undo everything that the create does.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- raw: accept VLAN tagged packets.
The power profile requires using VLAN priority tagged packets. The tags
may or may not reach the application. This commit adds code to handle the
tags in the case that they do appear.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- ptp4l: update the configuration files with the new defaults and sections
Update the configuration files for gPTP and default so that they show more of
the options as well as use suitable defaults for each style.

-v2
* Add [global] to gPTP.cfg

[ RC - also add logging_level, verbose, and use_syslog
     - use time_stamping, not timestamping ]

Signed-off-by: Jacob Keller <jacob.e.keller@intel.com>
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- ptp4l: per-port configure settings
this patch enables the configuration file to define per-port specific
configuration options via the use of simple INI style syntax with headings for
each port created via [device_name] syntax

[ RC - correct scanning of the port string
     - remove '=' from scan format string ]

Signed-off-by: Jacob Keller <jacob.e.keller@intel.com>
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- ptp4l: create [global] INI style config header
This patch creates a global heading for an INI style configuration file. The
parser allows the default state to be global without the header as well as to
use global. A future patch will enable per-port configuration settings.

-v2
* Forgot a "\n" in the string comparison so that global mode wasn't recognized

Signed-off-by: Jacob Keller <jacob.e.keller@intel.com>

- ptp4l: build the interfaces from cmdline after parsing options
this patch causes interfaces to be built after the options have finished
scanning rather than during option parsing. This removes the ability to
specify on the command line separate transport, or dm modes for each port.
Instead this functionality will be added in a follow-on patch via the
configuration file

Signed-off-by: Jacob Keller <jacob.e.keller@intel.com>

- ptp4l: read configuration file immediately after scanning options
This patch moves the call to read the configuration file until just after
the options have finished parsing. This is for future patches that will allow
configuration file to enable ports with specific settings

Signed-off-by: Jacob Keller <jacob.e.keller@intel.com>

- ptp4l: add pod parser to separate logic from scan_line
this patch extracts the pod parser from the scan_line function in order to
simplify the code for future patches

Signed-off-by: Jacob Keller <jacob.e.keller@intel.com>

- ptp4l: add override flag in cfg_settings to apply cmdline options
This patch adds a flag field for setting overflag flags so that command line
options override any value in the configuration file. This will be done by
ensuring the flags set whether the config parser accepts the values specified.
This patch also streamlines the handling for the slave only command line
option, as it no longer needs special treatment.

-v2
* Minor change to fix merge with previous patch

Signed-off-by: Jacob Keller <jacob.e.keller@intel.com>

- ptp4l: add missing options to config file
This patch adds support to the configuration file for all of the options
specified on the command line.

-v2
* Fix string length to account for null byte
* Add PRINT_LEVEL_MIN/MAX defines

Signed-off-by: Jacob Keller <jacob.e.keller@intel.com>

- ptp4l: Allow per-port customized port defaults
this patch allows each port to maintain its own pod structure since it is only
used in ports. This will allow the user to configure any special settings per
port. It takes a copy of the default pod, and a future patch will allow the
configuration file to set per-port specific changes

-v2
* Minor change to fix merge with previous patch

Signed-off-by: Jacob Keller <jacob.e.keller@intel.com>

- ptp4l: new config_create_interface function
this patch adds a support function that creates a new port based on the
current default settings and adds it to the iface list. The function returns
the index into the cfg where the port was created. If a port is attempted to
be created multiple times, future attempts just return the index

-v2
* Move assignment of pointer into array after bounds check

[ RC - fix off by one return code from config_create_interface ]

Signed-off-by: Jacob Keller <jacob.e.keller@intel.com>
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- ptp4l: change interface name into static array
In order to support future settings which allow for the configuration of
per-port data, this patch modifies the interface name to be a static array
instead of a pointer.

-v2
* Minor edit for merge with previous patch

Signed-off-by: Jacob Keller <jacob.e.keller@intel.com>

- ptp4l: move port default values into cfg_settings
move the dm, timestamping, and transport settings into the cfg_settings, and
treat them as defaults for new ports.

Signed-off-by: Jacob Keller <jacob.e.keller@intel.com>

- ptp4l: make ds and pod part of cfg_settings
make ds and pod static inside cfg_settings instead of created via pointers.
Also statically initialize the defaults.

Signed-off-by: Jacob Keller <jacob.e.keller@intel.com>

- ptp4l: pull iface into the configure settings
this patch modifies the ptp4l.c and config settings so that the iface list is
inside the cfg_settings structure

-v2
* Moved "struct interface" into config.h

Signed-off-by: Jacob Keller <jacob.e.keller@intel.com>

- ptp4l: pass struct interface directly instead of passing it's sub arguments
the port_open function takes a large number of command options, a few of which
are actually all values of struct interface. This patch modifies the port_open
call to take a struct interface value instead of all the other values. This
simplifies the overall work necessary and allows for adding new port
configuration values by appending them to the struct interface

Signed-off-by: Jacob Keller <jacob.e.keller@intel.com>

- ptp4l: remove timestamping as a per-port configuration option
The current code for the timestamping mode does not allow interfaces to have
separate timestamping modes. This is (probably) due to hardware timestamping
mode being required on all ports to work properly.

This patch removes the timestamping field in the struct iface, and makes it a
clock variable which is really what the mode does anyways. Ports get passed
the timestamping mode but no longer appear as though they are separate.

Signed-off-by: Jacob Keller <jacob.e.keller@intel.com>

- ptp4l: correct print global set functions
this patch updates the global set functions to allow the user to set the
proper value instead of only being allowed to enable (or disable) a particular
feature. The new patch allows the function to specify exactly what they want
the value to be.

This patch also clarifies what -q and -v do by removing mention of quiet mode
and verbose mode. It is easy for a user to confuse and assume that -q disables
-v when this is not true.

Signed-off-by: Jacob Keller <jacob.e.keller@intel.com>

- ptp4l: Change command line options to better match meaning
modify the command line options to make better sense of what each one does.
Ignore previous restriction on disallowing different options on the same
letter with different case.

the purpose of this patch is to simplify the meaning of some very confusing
options (-z for legacy, -r for hardware timestamps, -m for slave)

While there are legacy issues involved with changing options around, it is
important for the user to be able to quickly understand and make fewer
mistakes regarding the various command line options

Signed-off-by: Jacob Keller <jacob.e.keller@intel.com>

- Append the follow up information TLV when enabled.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Convert the follow up info tlv to and from network byte order.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add a configuration option to control the follow up info TLV.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Append the path trace list to announce messages when enabled.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Keep the path trace list up to date.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Filter incoming announce messages according to the path trace rule.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Enforce a length limit on an incoming path trace list.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add storage fields for the path trace list.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add a configuration option to control the path trace mechanism.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- raw: return the length of the PTP message on receive.
The upper layer code will be confused by the extra fourteen byte length
of the Ethernet header.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add a very simple management client.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Make the function to generate a clock identity a global function.
This will allow code reuse in the management client.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Place the sk_ globals into their proper source module.
This will allow reusing sk.c in the management client program.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Move the protocol version macro to a public header.
This will be needed by the management client program.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Answer all management requests with 'not supported'
Our management interface is not yet terribly useful,
but at least we are honest about it.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Provide a method to send a management error status message.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Provide a port method to allocate a management message reply.
This function will be needed for both positive replies and error status
messages.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Adding missing forward declaration.
Including port.h without clock.h produces a compilation error.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Filter port management messages by the target port number.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add a functional framework to manage the clock and its ports.
This commit only adds support for forwarding the management messages.
The actual local effects of the management commands still need to be
implemented.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Convert the management error status TLV to and from host byte order.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Convert the management TLV to and from host byte order.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add macros and enums for the various TLV codes.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add hooks for converting TLV values to and from host byte order.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Convert TLV type and length to host byte order on transmit.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Convert TLV type and length to host byte order on receive.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Remove unused macro.
Ever since 0afedd79, the DEFAULT_PHC macro is obsolete.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add a default configuration file for 802.1AS aka gPTP.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add an option to assume two step messaging.
Some commercial 802.1AS switches do not feel obliged to set the two step
flag. When we try to synchronize to their apparent one step sync messages,
nothing good happens. This commit adds a global option to work around the
issue.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Replace hard coded logMinPdelayReqInterval with configuration option.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add a configuration option to support 802.1AS only hardware.
Some of the time stamping hardware out there only recognizes layer 2
packets, and these do not work without changing the receive filter in
the SIOCSHWTSTAMP request.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add a configuration option to specify the L2 MAC addresses.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add the transport specific field into the config file.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Drop incoming packets on transportSpecific mismatch.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add transmit support for the transportSpecific field
Add transportSpecific parameter to config file parser
Set transportSpecific field in message headers as using the configuration (default to 0)

[ RC - reduced this patch to just the addition of the field ]

Signed-off-by: Delio Brignoli <dbrignoli@audioscience.com>
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Fix typo in the readme's driver support matrix.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Define CC = $(CROSS_COMPILE)gcc in makefile and add EXTRA_CFLAGS, EXTRA_LDFLAGS
Signed-off-by: Delio Brignoli <dbrignoli@audioscience.com>

- Update the readme file for Linux kernel 3.5.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Bring the driver support matrix up to date with Linux version 3.4.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Correctly handle a negative log message interval.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Prevent message buffer corruption on receive.
An oversize incoming packet might overwrite the reference counter in a
message. Prevent this by providing a buffer large enough for the largest
possible packet.

This will also be needed to support TLV suffixes.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Update data sets after loss of foreign master.
If an announce timeout occurs on a port, and no other port is slaved, then
the clock must become a grand master by default.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Fix unsafe reference of foreign master announce message.
When computing a port's best foreign master, we make use of a message
reference that possibly might have been dropped by calling msg_put in
the fc_prune subroutine. This commit fixes the issue by copying the
needed data from the message before pruning.

[ Actually, since msg_put only places the message into a list without
  altering its contents, there was no ill effect. But using a message
  after having released it is just plain wrong. ]

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Remove unneeded newline from pr_info message.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Recover from lost link when running in slave only mode.
Under Linux, when the link goes down our multicast socket becomes stale.
We always poll(2) for events, but the link down does not trigger any event
to let us know that something is wrong. Once the port enters master mode
and starts announcing itself, the socket throws an error. This in turn
causes a fault, and we reopen the socket when clearing the fault.

However, in the case of slave only mode, if the port is listening then
it will never send, discover the link error, or repair the socket. This
patch fixes the issue by simply reopening the socket after an announce
timeout.

[ Another way would be to use a netlink socket, but that would add too
  much complexity as it poorly matches our port/interface model. ]

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Fix memory leak, reference counting, and list handling in message code.
The message code is horribly broken in three ways.

1. Clearing the message also sets the reference count to zero.
2. The recycling code in msg_put does not test the reference count.
3. The allocation code does not remove the message from the pool,
   although this code was never reached because of point 2.

This patch fixes the issues and also adds some debugging code to trace
the message pool statistics.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Log the relative time rather than the process identification.
When diagnosing a log file, it can be useful to know the relative time
between the log entries. In contrast, the PID is mostly useless, since
the program does not ever fork.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Tell how to get the sources in the readme file.
Also fix a poorly worded sentence in the system requirements.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Fix the TLV structure declaration for actual use later.
This structure is not very useful for message parsing. This commit fixes
the declaration in preparation for TLV handling.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Refactor the post receive method to check the length first.
This patch is in preparation for handling the suffix TLV data. We will
need to use the structure size more than once.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add declarations for the signaling and management message types.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add the suffix field onto the general messages.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- ptp4l: use ethtool operation to double check PHC
If the new ethtool operation is supported, then use it to verify that the PHC
selected by the user is correct. If the user doesn't specify a PHC and ethtool
is supported then automatically select the PHC device.

If the user specifies a PHC device, and the ethtool operation is suppported,
automatically confirm that the PHC device requested is correct. This check is
performed for all ports, in order to verify that a boundary clock setup is
valid.

The check for PHC device validity is not done in the transport because the
only thing necessary for performing the check is the port name. Handled this
in the port_open code instead.

Signed-off-by: Jacob Keller <jacob.e.keller@intel.com>

- ptp4l: throw a fault for other port event failures
this patch makes sure every function is checked for a negative return value
and ensures that a fault is detected when these fail

-v2-
* Fixed only check the ones with return value

-v3-
* Modified the delay_req functions to return 0 on nonfault cases

Signed-off-by: Jacob Keller <jacob.e.keller@intel.com>

- Clamp maximum adjustment to numerical limit.
On 32 bit platforms, a PHC driver might allow a larger adjustment than
can fit into the 'long' type used in the clock_adjtime interface. This
patch fixes the issue by using the smaller of the two maxima.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Make the PI constants configurable.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add transport over UDP IPv6.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Use the peer transmission methods in the port logic.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- raw: Implement the peer transmission option.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- udp: Implement the peer transmission option.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Expand the transport layer interface with a peer transmission method.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Throw a fault when multiple peers are detected.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add a helper function to compare message source ports.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add a command line option to select the peer delay mechanism.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Expose the peer delay flavors in their own header file.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Implement the port peer delay mechanism.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add a clock method to accept a peer delay estimate from a slave port.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add port fields to remember peer delay messages.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add the peer delay messages into the message layer.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Fix a misspelled field name.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Warn when receiving delay requests on a peer to peer port.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Let each interface use its own transport.
This will allow running a boundary clock that bridges different kinds of
PTP networks.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- raw: fix filter reject for non 1588 frames
Signed-off-by: Eliot Blennerhassett <eblennerhassett@audioscience.com>

- Output an error message when no interface is specified
Signed-off-by: Christian Riesch <christian.riesch@omicron.at>

- Show every port state transition, including (re)initialization.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Automatically clear any port fault after fifteen seconds.
The current port code is very defensive. As the code now stands, we throw
a fault whenever we cannot send or receive a packet. Even a downed link
on an interface will cause a port fault.

This commit adds a very simple minded way of clearing the faults. We just
try to enable the port again after waiting a bit.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Rationalize the port reset logic.
This commit makes each pair of port functions, open/close and
initialize/disable, balance each other in how they allocate or free
resources. This change lays some ground work to allow proper fault
handling and disable/enable logic later on.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add a missing doxygen comment to the main program.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Remove a useless variable from the file descriptor array data type.
It was a cute idea to have the raw Ethernet layer use just one socket,
but it ended up not working on some specific PTP time stamping hardware.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add a method to remove a port from the clock's polling array.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Throw a fault event if the BMC algorithm fails.
That makes more sense than re-initializing, perhaps.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Make the number of transmit time stamp retries configurable.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Rework the configuration file interface.
This change will make it easier to extend the configuration file contents
to include arbitrary new data structures.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add a transport over raw Ethernet packets.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add a header defining the Ethernet header and MAC addresses.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Leave some headroom in the message buffers.
This room will be used by the Layer 2 protocols.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Improve error reporting on receive path.
In some error cases, no message is logged. Now we always complain loudly
when an error occurs.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add missing release method to the UDPv4 transport.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Move some sharable socket code into its own source file.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Clean up dynamic fields when closing a port.
In the course of development we added more and more allocations into the
port code without freeing them on close. We do not yet call the close
function, so there was never an issue. Once we start to reset the ports,
to clear faults for example, then we will need this.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Make the transport layer more opaque.
Although the UDP/IPv4 layer does not need any state per instance (other
than the two file descriptors), the raw Ethernet layer will need this.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Clear out saved time stamps after setting the clock.
When we create a discontinuity in the clock time, we must avoid mixing
local time stamps from before and after the jump.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Do not calculate the path delay without valid sync time stamps.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add time value operations to clear to zero and test for zero.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Throw an error at the port level on missing transmit time stamps.
We always wait for the transmit time stamp after sending an event message.
Thus a missing time stamp is clearly a fault, even if the hardware can
only handle one time stamp at a time.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Warn loudly whenever event messages are missing time stamps.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Fix segfault after missing transmit time stamp.
If the networking stack fails to provide a transmit time stamp, then we
might feed uninitialized stack data to the CMSG(3) macros. This can result
in a segfault or other badness.

The fix is to simply clear the control buffer in advance.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Remove silly and incorrect error message.
Commit 32133050 introduced a bug that gives a bogus error message on
every 'general' (non-event) PTP packet. If we want to catch missing
time stamps, then it has to occur at the port level.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Verbosely identify the port and message after network errors.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Be more verbose about errors on the receive path.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Initialize the time properties data set.
We can reuse the same function that sets the data sets in case of
becoming grand master.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Fix compilation for uclinux toolchain lacking <sys/timerfd.h>
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Clean up all of the binaries for the 'distclean' target.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add a utility program to set driver level time stamping policy.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add a tool to synchronize the system time from a PTP hardware clock.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Go ahead and use a negative path delay estimate.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Implement timeouts with log seconds less than zero.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add command line options to control the logging destination.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Print messages to the syslog by default.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Fix the port finite state machine.
The state machine needs to know whether a new master has just been
selected in order to choose between the slave and uncalibrated states.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Fix the BMC state decision algorithm.
We failed to make the distinction between plain old "better" and
"better by topology," but the BMC algorithm really counts on this.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Fix wrong result from the best master clock algorithm.
The BMC should never return 'faulty', and I don't know what I was thinking
here, so let's just fix it.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Differentiate the BMC related logging from the synchronization logging.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Fix typo in the initial priority field of the parent data set.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- uClinux: provide missing system calls.
The timerfd calls are missing from Sourcery CodeBench Lite 2011.09-23.
We can remove this code once these calls are properly integrated into a
current tool chain.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Make use of the configuration file for the port data set.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Make use of the configuration file for the default data set.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add code to read a configuration file.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add a command line option to set the message level.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Print the synchronization statistics at the information level.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Issue a warning when the path delay turns out negative.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Ignore messages from ourselves and from the wrong domain.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Do not print debug messages by default.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Fix message leak in the port event handler.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Do not treat signaling and management messages as errors.
Instead we just ignore them for now.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Make the slave only mode a non-default option.
Now that we have the master code in place, there is no longer any need
to restrict ourselves.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Implement the master sync timer and message.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Fix delay response message format.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Group the sequence numbers together in one structure.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Implement the master announce timer and message.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Provide methods to obtain a clock's parent and time properties data sets.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Implement the port master qualification timer.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Clear all timers when changing port state.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add an access method for a clock's currentDS.stepsRemoved.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Introduce an event recommending the grand master state.
We already have a grand master state. Adding this event will simplify the
overall logic, since it will avoid the silly requirement to set the
qualification timeout to zero when entering the grand master state.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Implement the synchronization events.
This will allow a port to get from the uncalibrated into the slave state.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- udp: Use the bind to device socket option.
Without this Linux specific option, multicast packets arrive on one
interface are delivered by the kernel to all others.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- udp: turn off multicast loop back.
This option is on by default, but we don't want or need it.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- udp: Use the IP_ADD_MEMBERSHIP socket option.
For some reason, MCAST_JOIN_GROUP is not working under uClinux. We can
just stick with the more traditional method.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- udp: bring a warning or error if the driver changes our hwtstamp options.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- udp: use the message logging facility instead of stdio.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Rename the main program to something better sounding.

- Do not generate dependencies if we are going to clean.

- Reject negative path delay.
If the path delay comes out negative, then something is amiss. In this
case, we just print a warning and ignore the path delay estimate.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- udp: wait longer for transmit time stamps
Some hardware is a bit pokey. We now wait forever on EINTR and just a
little bit on EAGAIN.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Let the clock servo know about the expected time stamp quality.
If software time stamping is to be used, then the servo will want to
have appropriate filtering.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add a bunch more documentation.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Select the system clock whenever legacy hardware time stamps are used.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Smooth the path delay estimate with a moving average.
This is really just a first attempt using a hard coded length. Probably
it will be necessary to let the length be configurable and/or adaptable.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Use the timeout table for the delay request messages.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add a time out table for delay requests.
We can pre-compute a table of suitable values in order to simplify the
run time random delay selection.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Use the correct slave-only clock class.
If we are going to be slave-only, then we should also advertise the
correct clock class.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Bind transmitted packets to the port's network interface.
Even though the MCAST_JOIN_GROUP socket option includes the interface
index, this applies to the received packets only. To bind the outgoing
packets to a particular interface, the IP_MULTICAST_IF option is needed.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Link it all together, but in slave-only mode.
Since the master implementation is still lacking, we will just keep
the slave-only flag hard coded for now.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Implement the port layer.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Implement the PTP clock.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- phc: Add a method to query the maximum adjustment.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add the main program.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Fix a misplaced doxygen comment.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add an abstract time value type.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add a modular clock servo interface with a PI controller.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add a message layer.
Note that only some of the message types are implemented.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add transport over UDP IPv4.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add utility functions for obtaining human readable strings.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add the best master clock algorithm.
This commit also introduces clock and port objects, but only with the
minimal interface needed by the BMC.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add the state machine.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add functions for logging messages in kernel style.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add code to open and close PHC devices.
Signed-off-by: Richard Cochran <richardcochran@gmail.com>

- Add the license, a readme, and some header files.
This commit gets the project off to a good start. The header files provide
some basic data types and definitions.

The README file will most probably grow over time.

Signed-off-by: Richard Cochran <richardcochran@gmail.com>
