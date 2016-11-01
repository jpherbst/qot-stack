# An End-to-end Quality of Time (QoT) Stack for Linux #

Having a shared sense of time is critical to distributed cyber-physical systems.
Although time synchronization is a mature field of study, the manner in which
Linux applications interact with time has remained largely the same --
synchronization runs as a best-effort background service that consumes resources
independently of application demand, and applications are not aware of how
accurate their current estimate of time actually is.

Motivated by the Internet of Things (IoT), in which application needs are
regularly balanced with system resources, we advocate for a new way of thinking
about how applications interact with time. We propose that Quality of Time (QoT)
be both *observable* and *controllable*. A consequence of this being that time
synchronization must now satisfy timing demands across a network, while
minimizing the system resources, such as energy or bandwidth.

Our stack features a rich application programming interface that is centered
around the notion of a *timeline* -- a virtual sense of time to which each
applications bind with a desired *accuracy interval* and *minimum resolution*.
This enables developers to easily write choreographed applications that are
capable of requesting and observing local time uncertainty.

The stack works by constructing a timing subsystem in Linux that runs in
parallel to timekeeping and POSIX timers. In this stack quality metrics are
passed alongside time calls. The Linux implementation builds upon existing open
source software, such as NTP, LinuxPTP and OpenSplice to provide a limited
subset of features to illustrate the usefulness of our architecture.

# License #

Copyright (c) Regents of the University of California, 2015.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

# Installation and Usage #

Please refer to the [wiki](https://bitbucket.org/rose-line/qot-stack/wiki).

# Contributors #

**University of California Los Angeles (UCLA)**

* Mani Srivastava
* Sudhakar Pamarti
* Fatima M. Anwar
* Andrew Symington
* Hani Esmaeelzadeh
* Amr Alanwar
* Paul Martin

**Carnegie-Mellon University (CMU)**

* Anthony Rowe
* Raj Rajkumar
* Adwait Dongare
* Sandeep Dsouza

**University of California San Diego (UCSD)**

* Rajesh Gupta
* Sean Hamilton
* Zhou Fang

**University of Utah**

* Neal Patwari
* Thomas Schmid
* Anh Luong

**University of California Santa Barbara (UCSB)**

* Joao Hespanha
* Justin Pearson
* Masashi Wakaiki
* Kunihisa Okano
* Joaquim Dias Garcia

=======
# Overview #

This project is intended for developers, and so it presumes a certain working knowledge of embedded Linux. The general idea is to have BeagleBones fetch a Linux kernel and device tree over TFTP from a controller, and then mount an NFS share at the root file system. In this was we don't have to insert and eject many microsd cards, and we are guaranteed to have a consistent version of firmware across all nodes.
In case, a more simpler version of the setup is desired, please refer to the section on the controller setup and then directly refer to the section on the Standalone SD card setup. For the NFS based setup, please skip the section on the standalone setup.

![qot-setup.png](https://bitbucket.org/repo/5Eg8za/images/2069891140-qot-setup.png)

There are three key devices in the system:

1. Controller (Ubuntu 15.04, x86_64-linux-gnu) - NAT router, DCHP server, NFS server, TFTP server
1. Host (Ubuntu 15.04, x86_64-linux-gnu) - Where you do your development
1. Slaves (Ubuntu 15.04, arm-linux-gnueabihf) - The actual BeagleBones

Since the synchronization algorithm is based on wired PTP, for the qot-stack to work effectively you will need a IEEE 1588v2 compliant network. The slaves have a PTP-compliant Ethernet adapter, and Linux supports hardware time stamping out of the box. Our version of PTP is derived from the linuxptp project.

Robert Nelson's `bb-kernel` project provides almost everything we need to build a suitable kernel for the BeagleBone Black. The idea will be to check this project out on the controller and build a kernel for the slaves. We will export this project along with the rootfs over NFS. The reason for this is that we can NFS-mount it at /export on our host environment and cross-compile and install kernel modules and user-space applications very easily.

So, to summarize, you will end up having this on directory (/export) on your central controller:

```
- /export
  - bb-kernel : kernel build script
  - rootfs : location of the root file syste,
  - tftp : contains kernel image and device trees
```

And, you will NFS mount this on each host. When you compile the kernel you should use the cross-compiler that is automatically downloaded by the kernel build script. However, when you compile applications you need to use the `arm-linux-gnueabihf` compiler in the Ubuntu. Also, it is really important that the version of this GCC compiler matches the version deployed on the slaves (currently 4.9.2). The reason for this is that different compilers have different libc versions, which causes linker errors that are very tricky to solve.

# Table of contents #

[TOC]

  
# Controller preparation #

This section describes how to prepare your central controller. However, in order to do it must make some assumptions about your controller. In order to be used as a NAT router, your controller must have at least two interfaces. I'm going to assume the existence of these two adapters, and you will need to modify the instructions if they are different. You can use the network configuration manager in Ubuntu to configure them accordingly.

1. eth0 - connected through a LAN to the slaves (static IP 10.42.0.1 and only local traffic)
2. wlan0 - connected to the internet (address from)

Install the necessary system applications

```
$> sudo apt-get install build-essential git cmake cmake-curses-gui gawk flex bison perl doxygen u-boot-tools 
$> sudo apt-get install nfs-kernel-server tftpd-hpa isc-dhcp-server ufw
```

EVERYTHING IN THIS SECTION MUST BE EXECUTED ON THE CONTROLLER.

## STEP 1 : Install the root filesystem and kernel  ##

First, create one important directory:

```
$> sudo mkdir /export
```

Then, download the qot bundle and decompress it to the correct location (as root)

```
$> su -
$> cd /export
$> wget https://bitbucket.org/rose-line/qot-stack/downloads/qot-bundle.tar.bz2
$> tar -xjpf qot-bundle.tar.bz2
$> rm -rf qot-bundle.tar.bz2
```

Then, checkout the bb-kernel code.

```
$> su -
$> cd /export
$> git clone https://github.com/RobertCNelson/bb-kernel.git -b 4.1.12-bone-rt-r16
$> cd bb-kernel
```
Then, build the kernel using the QoT configuration. To do this, type the following command. When the Linux menu configuration appears, click the LOAD option at the bottom right and choose `/export/qot.config` as the path to the configuration file.

```
$> ./build_kernel.sh
```

Once the kernel has configured, add the following two lines to the bottom of `/export/bb-kernel/system.sh` in order to set variables for your TFTP and root file system (so that the install script knows where to put them).

```
ROOTFS=/export/rootfs
TFTPFS=/export/tftp
```

Then, run the install script while you are in `bb-kernel`.

```
$> /export/install_netboot.sh
```

Now, you have a working kernel

# Standalone SD Card Setup #

Skip this section, if you are setting up an NFS based setup.

1. Download the SD card image from this [link](https://drive.google.com/file/d/0B5sYz4zKsYSaQk1MWTlicGdFcU0/view?usp=sharing).
2. The SD card image contains all the relevant modules and libraries, required for the Linux QoT-Stack. Install the SD card image onto an SD card (minimum size 8 GB), using available free software (needs to be elaborated). 
3. Insert the SD card into the beaglebone black, and boot the node. 
4. Figure out the IP address assigned to the node by: (i) Connect the beaglebone to the host using the supplied USB cable (ii) SSH into the beaglebone using ssh root@192.168.7.2 (iii) Figure out the IP address by using ifconfig -eth0 (iv) Add the public key of your host account (available at ~/.ssh/id-rsa.pub) to the the following file on the node `~/.ssh/authorized_keys` (v) Exit the ssh connection and disconnect the USB cable. You should now be able to ssh into the node using ethernet and the IP address of the node (ssh root@ip-addr)
5. Copy the lates versions of the stack components to the node: (i) Navigate to the directory with the qot-stack. Edit the Makefile in the directory. (ii) Replace the field IPADDR with the ip-address of the node (obtained from the previous step) (iii) do a `make` followed by a `make install_sd`. All the latest versions of the stack componenets and libraries are now on the node.
6. ssh into the node (ssh root@ip-addr)
7. Enter the following commands in the remote terminal on the node: `ldconfig`, `depmod -a`
8. Load the modules: `capes BBB-AM335X`

Skip the networking steps, and proceed to the section on configuring the QoT-Stack


## STEP 2 : Networking  ##


In order to enable Network File Sharing (NFS) across the Controller and beaglebone devices, we need to setup a DHCP Server that will dynamically assign IP addresses to the controller and beaglebone devices.
You can setup a DHCP server on a router or a server with your own desired networking details. The end goal is to make the DHCP Server assign static IP addresses to the controller and beaglebones using their unique MAC addresses.

You can follow the two steps below, if you do not have the means to setup your own DHCP Server, instead you can make the Controller behave as a DHCP Server.
However, these steps are complex and require proper configuration. Most likely, they will not work depending upon your Linux version or other reasons.

### STEP 2a : Configure DHCP  ###

The Ubuntu host needs to act as a DHCP server, assigning IPs to slaves as they boot. I personally prefer to define each of my slaves in the configuration file so that they are assigned a constant network address that is preserved across booting. Edit the `/etc/default/isc-dhcp-server` file to add the interface on which you wish to serve DHCP requests:

```
INTERFACES="eth0"
```

Now, configure the DHCP server and add an entry for each slave node in `/etc/dhcp/dhcpd.conf` . The configuration below assigns the IP `10.42.0.100` to the single slave with an Ethernet MAC `6c:ec:eb:ad:a7:3c`. To add more slaves, just add additional lines to this file.

```
subnet 10.42.0.0 netmask 255.255.255.0 {
 range 10.42.0.100 10.42.0.254;
 option domain-name-servers 8.8.8.8;
 option domain-name "roseline.local";
 option routers 10.42.0.1;
 option broadcast-address 10.42.0.255;
 default-lease-time 600;
 max-lease-time 7200;
}

host alpha { hardware ethernet 6c:ec:eb:ad:a7:3c; fixed-address 10.42.0.100; }
```

Then, restart the server:

```
$> sudo service isc-dhcp-server restart
```

### STEP 2b : Configure NAT ###

In the file `/etc/default/ufw` change the parameter `DEFAULT_FORWARD_POLICY`

```
DEFAULT_FORWARD_POLICY="ACCEPT"
```

Then, configure `/etc/ufw/sysctl.conf` to allow ipv4 forwarding.

```
net.ipv4.ip_forward=1
#net/ipv6/conf/default/forwarding=1
#net/ipv6/conf/all/forwarding=1
```

Then, add the following to `/etc/ufw/before.rules` just before the filter rules (starts with `*filter`).

```
# NAT table rules
*nat
:POSTROUTING ACCEPT [0:0]
-A POSTROUTING -s 10.42.0.0/24 -o wlan0 -j MASQUERADE
COMMIT
```

Finally, restart the uncomplicated firewall.

```
$> sudo ufw disable && sudo ufw enable
```

You'll also need to add a rule that allows all traffic in on `eth0` so that the slaves can access services.


```
$> sudo ufw allow in on eth0
```

## STEP 4 : Configure TFTP  ##

Then, edit the `/etc/default/tftpd-hpa` file to the following:

```
TFTP_USERNAME="tftp" 
TFTP_DIRECTORY="/export/tftp" 
TFTP_ADDRESS="10.42.0.1:69"   % This is the IP Address of the Controller %
TFTP_OPTIONS="-s -c -l"
```

Then, restart the server:

```
$> sudo service tftpd-hpa restart
```

## STEP 5 : Configure NFS  ##

Edit the NFS `/etc/exports` on the share the `/export` directory 

```
/export 10.42.0.2(rw,sync,no_root_squash,no_subtree_check) % Enter IP Addresses of the beaglebone devices here %
/export 10.42.0.3(rw,sync,no_root_squash,no_subtree_check)
```

Then, restart the server:

```
$> sudo service nfs-kernel-server restart
```

## STEP 6 : Netboot a test slave  ##

To boot the qot stack you'll need a well-formed microsd card. Please note that fdisk is a partitioning tool, and if you do not specify the correct device, you run the risk of formatting the incorrect drive. So, please use `tail -f /var/log/syslog` before inserting the card to see which device handle (/dev/sd?) is assigned by udev when the card is inserted into your PC.

Then run the following:

1. Start fdisk to partition the SD card: `sudo fdisk /dev/sd?`
1. Type `o`. This will clear out any partitions on the drive.
1. Type `p` to list partitions. There should be no partitions left.
1. Type `n`, then `p` for primary, `1` for the first partition on the drive, `enter` to accept the default first sector, then `+64M` to set the size to 64MB.
1. Type `t` to change the partition type, then `e` to set it to W95 FAT16 (LBA).
1. Type `a`, then `1` to set the bootable flag on the first partition.
1. Type `n`, then `p` for primary, `2` for the second partition on the drive, and `+2GB` to set the size to 2GB.
1. Type `n`, then `p` for primary, `3` for the third partition on the drive, and `enter` to select the remaining bytes on the drive.
1. Write the partition table and exit by typing `w`.
1. Initialise the filesystem for partition 1: `mkfs.vfat -F 16 /dev/sd?1 -n boot`
1. Initialise the filesystem for partition 2: `mkfs.ext4 /dev/sd?2 -L rootfs`
1. Initialise the filesystem for partition 3: `mkfs.ext4 /dev/sd?3 -L user`

Now, install the bootloader:

```
$> su -
$> sudo mount /dev/sd?1 /mnt
$> cd /mnt
$> wget https://bitbucket.org/rose-line/qot-stack/downloads/MLO
$> wget https://bitbucket.org/rose-line/qot-stack/downloads/u-boot.img
$> wget https://bitbucket.org/rose-line/qot-stack/downloads/uEnv.txt  % This file contains a field with the Controller's static IP address. Please replace it with your Controller's IP address%
$> cd /
$> umount /mnt
```

Because multiple BeagleBone nodes will be mounting the same filesystem on the controller, the `/etc/ssh` folder in the provided root file system has been replaced with a sym-link to `/mnt/ssh`. The second partition of the sd card will be mounted on `/mnt`, so create the ssh directory in that partition.

```
mount /dev/sd?2 /mnt
cd /mnt/
mkdir ssh/
cd ssh/
ssh-keygen -q -N '' -f ssh_host_rsa_key -t rsa
```

You will also need a directory for opensplice with a file [ospl.xml](https://bitbucket.org/rose-line/qot-stack/downloads/ospl.xml). You must also update `<NetworkInterfaceAddress>` (on line 18 of the file) with the IP address of the Beaglebone node you are installing this file on.

In /mnt:
```
mkdir opensplice
cd opensplice/
wget https://bitbucket.org/rose-line/qot-stack/downloads/ospl.xml
cd ~
umount /mnt
```

For now (until we can mount an sd card partition for the rootfs), this sym-link mechanism will be used to allow different nodes to have individual copies of files/directories.

Using this card you you should be able to net-boot a Rev C BeagleBone that is plugged into the same LAN as the controller. The four blue LEDs will light up sequentially until all are lit, and you should see the green LED on the Ethernet jack flicker as the kernel / device tree are downloaded from the controller. All four blue LEDS will then turn off for a few seconds while the kernel begins the boot process. They should then rapidly flick for a few more seconds before the classic heartbeat led begins. 

To determine the IP address that the controller offered, or debug any errors check the system log:

```
$> tail -f /var/log/syslog
```

If you can't find anything useful there, use a FTDI cable to inspect the u-boot process of the slave.

At this point you should be able to SSH into the slave device as root. However, he root password for this account is randomized by default. So, in order to SSH into the device you will need to add your RSA public key to `/export/rootfs/root/.ssh/authorized_keys`. To find your public key type:

```
$> cat  ~/.ssh/id_rsa.pub
```

If you get a 'No such file or directory found' error, then you need to first create your key pair with:

```
$> ssh-keygen -t rsa
```

# Build instructions #

Assuming that you have successfully built and net-booted the kernel, you can now setup your host environment to cross-compile kernel modules and  applications for the BeagleBone.

EVERYTHING IN THIS SECTION SHOULD BE CARRIED OUT ON THE HOST. IF YOU WANT, YOU CAN USE YOUR CONTROLLER TO BE THE HOST AS WELL (In this case, jump to Step 2).

## STEP 1 : Setup the /export share ##

Recall that we NFS-exported the `/export` directory from the controller. Create the same directory on your host.

```
$> sudo mkdir -p /export
```

Now, we will add the following line to the end of the host's `/etc/fstab` file to automount this central controller's NFS share when the machine is booted.

```
10.42.0.1:/export /export nfs rsize=8192,wsize=8192,timeo=14,intr
```

Reboot the host and you should see the rootfs, tftp and bb-kernel directories in the `/export` directory.

## STEP 2 : Check out the QoT stack code ##

Start by checking out the qot-stack code

```
$> git clone https://bitbucket.org/rose-line/qot-stack.git
$> cd qot-stack
```

Intialize the third-party code (OpenSplice and the DTC compiler)

```
$> git submodule init
$> git submodule update
```

## STEP 3 : Update the device tree compiler ##

Then, install an overlay for the device tree compiler. This compiles a new version of `dtc` which you can use to build overlays for the BeagleBone Black.

```
$> pushd thirdparty/bb.org-overlays
$> ./dtc-overlay.sh
$> popd
```

\*Note the version of dtc you have.

## STEP 4 : Build OpenSplice ##

Switch to the OpenSplice directory and pull the third party repo for the C++ bindings

```
$> pushd thirdparty/opensplice
$> git checkout -b v64 OSPL_V6_4_OSS_RELEASE
$> git submodule init
$> git submodule update
```

Configure and build the OpenSplice DDS library. The configure script searches for third party dependencies. The third party libraries ACE and TAO are only required for Corba, and in my experience introduce compilation errors. So, I would advise that you do not install them. 

```
$> ./configure
```

Assuming that you chose the build type to be armv7l.linux-dev, then you will see that a new script `envs-armv7l.linux-dev.sh` was created in the root of the OpenSplice directory. You need to first source that script and then build. The build products will be put in the ./install directory. 

```
$> . envs-armv7l.linux-dev.sh
$> make
$> make install
```

Then, add C++11 support by inserting the `-std=c++11` argument to the `CPPFLAGS` variable in `./install/HDE/armv7l.linux-dev/custom_lib/Makefile.Build_DCPS_ISO_Cpp_Lib` file. You will now need to recompile the C++ interface:

```
$> pushd install/HDE/%build%/custom_lib
$> make -f Makefile.Build_DCPS_ISO_Cpp_Lib
$> popd
$> popd
```

You now have a working OpenSplice distribution with C++11 support. This basically provides a fully-distributed publish-subscribe messaging middleware with quality of service support. This mechanism will be used to advertise timelines across the network. The slaves have their own arm versions of Java, ROS and OpenSplice 6.4 in the `/opt` directory of the rootfs, and whenever you SSH into a slave the `/etc/profile` script initializes all three for you. 

Note that whenever you run an OpenSplice-driven app you will need to set an environment variable `OSPL_URI` that configures the domain for IPC communication. This is described by an XML file, which is usually placed somewhere in your OpenSplice source tree. There are some default files. The slave rootfs is configured by default to find the XML configuration in /mnt/openxplice/ospl.xml, as the configuration needs to be different for each slave -- they have different IPs and thus different`<NetworkInterfaceAddress>` tag values. Instructions for installing this XML file are in Step 6

## STEP 5 : Build and install ##

Before building, make sure that you have followed the steps [here](https://bitbucket.org/rose-line/qot-stack/src/e856dbca2ddb1ecc83c0a14b743aa69d4b30bd93/src/test/?at=thorn16_refactor) for unit tests.

The entire project is cmake-driven, and so the following should suffice:

```
$> mkdir -p build % Do this in the top most project directory /qot-stack %
$> pushd build
$> ccmake ..
```

Make sure that CROSS_COMPILE is ON and that your INSTALL_PREFIX is `/export/rootfs/usr/local`.

```
$> make
$> sudo make install
$> popd
```

You may get this error:
```
WARNING: "qot_register" [/home/shk/bb/qot-stack/src/modules/qot_am335x/qot_am335x.ko] undefined!
WARNING: "qot_unregister" [/home/shk/bb/qot-stack/src/modules/qot_am335x/qot_am335x.ko] undefined!
```
while compiling. This is due to module files being in different directories. Copy over `Module.symvers` file to fix this error.
```
$> cp src/modules/qot/Module.symvers src/modules/qot_am335x/Module.symvers
```
Run `make` again and the warning should be gone.

This will install the QoT stack to your the NFS share located as `/export/rootfs`. It should be available instantly on the nodes.

After installing the user-space applications you might need to run `ldconfig` on the node.

## STEP 6: Install kernel modules ##

In the top most project directory (\qot-stack) run,

```
$> make
$> sudo make install
```

After installing the kernel modules you might need to run `depmod` on the nodes.

# Configuring the QoT stack #

Firstly, SSH into a node of choice:

If you type `capes` on the command line you should see four slots as empty:

```
root@arm:~# capes
 0: PF----  -1 
 1: PF----  -1 
 2: PF----  -1 
 3: PF----  -1 
```

When you installed the kernel module a DTBO file was copied to  `/export/rootfs/lib/firmware`. This is a device tree overlay file that tells the BeagleBone how to multiplex its I/O pins, and which kernel module to load after it has done so. To apply the overlay use the `capes` command.

```
$> capes BBB-AM335X
```

The output of the `capes` command should now be this:

```
root@arm:~# capes
 0: PF----  -1 
 1: PF----  -1 
 2: PF----  -1 
 3: PF----  -1 
 4: P-O-L-   0 Override Board Name,00A0,Override Manuf,BBB-AM335X
```

And the `lsmod` command should list two new kernel modules:

```
root@arm:~# lsmod
Module                  Size  Used by
qot_am335x              7121  0 
qot                     7655  3 qot_am335x
```

Normally modules are automatically loaded from the capes command. However, it is not always the case. You can manually load the modules using the commands,

```
root@arm:~# insmod qot
root@arm:~# insmod qot_am335x
```

If these commands, give you path error, do the following,

```
root@arm:~# cd /lib/modules/4.1.12-bone-rt-r16/kernel/drivers/misc
root@arm:~# insmod qot.ko
root@arm:~# insmod qot_am335x.ko
```

If you still have issues, do "dmesg" and see the logs for issues.

# Running PTP Synchronization on the QoT stack #

QoT Stack does synchronization in two steps. It first aligns the local core clock to the network interface clock. 
We need to enable ptp pin capabilities in order for this to work. You can check pin capabilities of various ptp devices using,

```
$> testptp -d /dev/ptp0 -c % This is the ethernet controller driver exposed as ptp clock %
$> testptp -d /dev/ptp1 -c % qot_am335x driver exposes the core as a ptp clock %
```

In the output, see that pin functionalities like external timestamping and interrupt trigger are enabled.

For the onboard ptp device for the ethernet controller (/dev/ptp0), the pin capabilities are not enabled by default. We need to patch a file (cpts.c) in the kernel at (`/export/bb-kernel/KERNEL/drivers/net/ethernet/ti/cpts.c`) to enable the pins. (You can use the `diff` and `patch` utilities to do this easily) Patch the file and commit the changes in the `KERNEL` repository.
The new file can be found at (https://bitbucket.org/rose-line/qot-stack/downloads/cpts.c) 

In `/export/bb-kernel/KERNEL`:
```
patch (...)  % patch the cpts.c file
git add drivers/net/ethernet/ti/cpts.c
git commit -m "patched cpts.c to add eth controller pin functionalities"
```

Then we need to create a git formatted patch, put it inside the the `/bb-kernel/patches/` directory, and point `patch.sh` to the patch file. This is because the kernel building scripts (`build_kernel.sh` and `rebuild.sh`) use patch.sh to apply all required patches before building the kernel.

```
git format-patch HEAD~ % creates a patch file with the changes from the previous commit
cd .. % to go back to /export/bb-kernel/
cp KERNEL/0001-patched-cpts.c-to-add-eth-controller-pint-functionali.patch \
   patches/beaglebone/phy/
```

Now edit `patch.sh` and find where the script enters the `/patches/beaglebone/phy/` directory. (Search for 'patches/beaglebone/phy' in the file) and add the following line:
```
${git} "${DIR}/patches/beaglebone/phy/0001-patched-cpts.c-to-add-eth-controller-pin-functionali.patch"
```

Now we are ready to rebuild the kernel.

In /export/bb-kernel:
```
$> ./tools/rebuild.sh
```

After the kernel is rebuilt, restart the beaglebone nodes, and run the following in one terminal,

```
$> phc2phc
```

Keep this running for the entire synchronization process.
Once the on-board Core clock and NIC are properly aligned (follow the logs of phc2phc to verify), we now need to synchronize clocks across different devices.

The `qotdaemon` application monitors the `/dev` directory for the creation and destruction of `timelineX` character devices. When a new device appears, the daemon opens up an ioctl channel to the qot kernel module query metadata, such as name / accuracy / resolution. If it turns out the character device was created by the qot module, a PTP synchronization service is started on a unique domain over `eth0`. Participating devices use OpenSplice DDS to communicate the timelines they are bound to, and a simple protocol elects the master and slaves. Right now, the node with the highest accuracy requirement is elected as master.

In order to create a timeline first, run helloworld application in a separate terminal,

```
$> helloworld
```

And then launch the synchronization daemon in another terminal:

```
$> qotdaemon -v
```

Repeat the synchronization step for other nodes as well, and then you will see multiple nodes -- bound to same timeline -- synchronize to each other.

# Development #

This code is maintained by University of California Los Angeles (UCLA) on behalf of the RoseLine project. If you wish to contribute to development, please fork the repository, submit your changes to a new branch, and submit the updated code by means of a pull request against origin/thorn16_refactor.

# Support #

Questions can be directed to Fatima Anwar (fatimanwar@ucla.edu).