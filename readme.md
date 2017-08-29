# The Quality of Time (QoT) Stack for Linux #

# Overview #
Having a shared sense of time is critical to distributed cyber-physical systems. Although time synchronization is a mature field of study, the manner in which time-based applications interact with time has remained largely the same --synchronization runs as a best-effort background service that consumes resources independently of application demand, and applications are not aware of how accurate their current estimate of time actually is.

Motivated by Cyber-Physical Systems (CPS), in which application requirements need to be balanced with system resources, we advocate for a new way of thinking about how applications interact with time. We introduce the new notion of Quality of Time (QoT) as the, *"end-to-end uncertainty in the notion of time delivered to an application by the system"*. We propose that the notion of Quality of Time (QoT) be both *observable* and *controllable*. A consequence of this being that time synchronization must now satisfy timing demands across a network, while minimizing the system resources, such as energy or bandwidth.

Our stack features a rich application programming interface (API) that is centered around the notion of a *timeline* -- a virtual sense of time to which applications bind with their desired *accuracy interval* and *minimum resolution* timing requirements. This enables developers to easily write choreographed applications that are capable of requesting and observing local time uncertainty.

The stack works by constructing a timing subsystem in Linux that runs parallel to timekeeping and POSIX timers. In this stack quality metrics are passed alongside time calls. The Linux implementation builds upon existing open-source software, such as NTP, LinuxPTP and OpenSplice to provide a limited subset of features to illustrate the usefulness of our architecture.

# License #

Copyright (c) Regents of the University of California, 2015.
Copyright (c) Carnegie Mellon University, 2017.
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
* Sandeep Dsouza
* Adwait Dongare

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

# Table of contents #

[TOC]

# Installation Overview #

This project is intended for developers, and so it presumes a certain working knowledge of embedded Linux. We provide setup instructions for the ARM-based Beablebone Black (BBB) embedded Linux platform, x86-based machines, and x86-based virtualized environments. 

For the BBB we describe two types of development setup scenarios: (1) an NFS-TFTP-based setup, and (2) a standalone SD-card based setup. For the BBB NFS-based setup, the general idea is to have BeagleBones fetch a Linux kernel and device tree over TFTP from a controller, and then mount an NFS share at the root file system. In this way we don't have to insert and eject many microsd cards, and we are guaranteed to have a consistent version of firmware across all nodes. Most of this documentation is dedicated to explaining this setup. For the BBB, in case, a more simpler version of the setup is desired, please refer to the section on the [Standalone SD card setup](#bbb-standalone-sd-card-setup). For the NFS-based setup, please skip the section on the standalone setup. If you are interested in setting up the stack for an x86-based machine, please skip to the [x86 section](#x86-standalone-setup). For setting up the QoT Stack for Linux in a virtualized environment (QEMU-KVM), please skip to the [virtualization-setup section](#qemu-kvm-virtual-machine-based-setup).

![qot-setup.png](https://bitbucket.org/repo/5Eg8za/images/2069891140-qot-setup.png)

## BBB NFS-TFTP-based development setup ##
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
  - rootfs    : location of the root file syste,
  - tftp      : contains kernel image and device trees
```

And, you will NFS mount this on each host. When you compile the kernel you should use the cross-compiler that is automatically downloaded by the kernel build script. However, when you compile applications you need to use the `arm-linux-gnueabihf` compiler in the Ubuntu. Also, it is really important that the version of this GCC compiler matches the version deployed on the slaves (currently 4.9.2). The reason for this is that different compilers have different libc versions, which causes linker errors that are very tricky to solve.

### Controller preparation ###

This section describes how to prepare your central controller. However, in order to do it must make some assumptions about your controller. In order to be used as a NAT router, your controller must have at least two interfaces. I'm going to assume the existence of these two adapters, and you will need to modify the instructions if they are different. You can use the network configuration manager in Ubuntu to configure them accordingly.

1. eth0 - connected through a LAN to the slaves (static IP 10.42.0.1 and only local traffic)
2. wlan0 - connected to the internet (address from)

Install the necessary system applications

```
$> sudo apt-get install build-essential git cmake cmake-curses-gui gawk flex bison perl doxygen u-boot-tools 
$> sudo apt-get install nfs-kernel-server tftpd-hpa isc-dhcp-server ufw
```

EVERYTHING IN THIS SECTION MUST BE EXECUTED ON THE CONTROLLER.

#### STEP 1 : Install the root filesystem and kernel  ####

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

Now, you have a working kernel. 

#### STEP 2 : Networking  ####


In order to enable Network File Sharing (NFS) across the Controller and beaglebone devices, we need to setup a DHCP Server that will dynamically assign IP addresses to the controller and beaglebone devices.
You can setup a DHCP server on a router or a server with your own desired networking details. The end goal is to make the DHCP Server assign static IP addresses to the controller and beaglebones using their unique MAC addresses.

You can follow the two steps below, if you do not have the means to setup your own DHCP Server, instead you can make the Controller behave as a DHCP Server. However, these steps are complex and require proper configuration. Most likely, they will not work depending upon your Linux version or other reasons.

##### STEP 2a : Configure DHCP  #####

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

##### STEP 2b : Configure NAT #####

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

#### STEP 4 : Configure TFTP  ####

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

#### STEP 5 : Configure NFS  ####

Edit the NFS `/etc/exports` on the share the `/export` directory 

```
/export 10.42.0.2(rw,sync,no_root_squash,no_subtree_check) % Enter IP Addresses of the beaglebone devices here %
/export 10.42.0.3(rw,sync,no_root_squash,no_subtree_check)
```

Then, restart the server:

```
$> sudo service nfs-kernel-server restart
```

#### STEP 6 : Netboot a test slave  ####

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
$> mount /dev/sd?2 /mnt
$> cd /mnt/
$> mkdir ssh/
$> cd ssh/
$> ssh-keygen -q -N '' -f ssh_host_rsa_key -t rsa
```

You will also need a directory for opensplice with a file [ospl.xml](https://bitbucket.org/rose-line/qot-stack/downloads/ospl.xml). You must also update `<NetworkInterfaceAddress>` (on line 18 of the file) with the IP address of the Beaglebone node you are installing this file on.

In /mnt:
```
$> mkdir opensplice
$> cd opensplice/
$> wget https://bitbucket.org/rose-line/qot-stack/downloads/ospl.xml
$> cd ~
$> umount /mnt
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

Now you have a working kernel setup. Skip the standalone SD card setup, and proceed to the QoT Stack Build Instructions section on building and configuring the QoT Stack.

## BBB Standalone SD Card Setup ##

Skip this section, if you are setting up an NFS-based setup.
1. Download the SD card image from this [link](https://drive.google.com/file/d/0B5sYz4zKsYSaQk1MWTlicGdFcU0/view?usp=sharing). 
2. The SD card image contains all the relevant modules and libraries, required for the Linux QoT-Stack. Install the SD card image onto an SD card (minimum size 8 GB), using available free software (needs to be elaborated). 
3. Insert the SD card into the beaglebone black, and boot the node. 
4. Figure out the IP address assigned to the node by: (i) Connect the beaglebone to the host using the supplied USB cable (ii) SSH into the beaglebone using ssh root@192.168.7.2 (iii) Figure out the IP address by using `ifconfig -eth0` (iv) Add the public key of your host account (available at ~/.ssh/id-rsa.pub) to the the following file on the node `~/.ssh/authorized_keys` (v) Exit the ssh connection and disconnect the USB cable. You should now be able to ssh into the node using ethernet and the IP address of the node (ssh root@ip-addr)

You now have a working kernel and you should be able to boot and ssh into the node over a network. Skip to the QoT Stack Build Instructions section.

## x86 Standalone Setup ##
Skip this section, if you are setting up a Beaglebone Black. Setting up an x86 machine is very straightforward. 

1. Install your favourite Debian-based distribution on your machine (preferred Ubuntu 14.04, as the setup instructions have been tested). 
2. Upgrade the kernel to your preferred version (v4.1 + preferred). (Instructions to do so can be found here https://askubuntu.com/questions/119080/how-to-update-kernel-to-the-latest-mainline-version-without-any-distro-upgrade)
3. Install the necessary system applications
```
$> sudo apt-get install build-essential git cmake cmake-curses-gui gawk flex bison perl doxygen u-boot-tools 
$> sudo apt-get install nfs-kernel-server tftpd-hpa isc-dhcp-server ufw
```

You should now have a working x86-based machine with a debian distribution and Linux kernel version of choice. Note that, the controller can also be used as a test-bed for the QoT Stack (assuming an x86-based controller).

## QEMU-KVM Virtual Machine-based Setup ##
Skip this section, if you are setting up a Beaglebone Black or an x86 Native Machine. This section describes how to setup QEMU-KVM on an x86 hosts, and follows up with the basic steps required to install a Linux virtual machine which is capable of supporting QoT functionality. 

### Step 1: Install QEMU-KVM ###
QEMU-KVM (for Kernel-based Virtual Machine) is a full virtualization solution for Linux on x86 hardware containing virtualization extensions (Intel VT or AMD-V). It consists of a loadable kernel module, kvm.ko, that provides the core virtualization infrastructure and a processor specific module, kvm-intel.ko or kvm-amd.ko. QEMU-KVM uses the QEMU emulator as an hypervisor to run virtual machines, while the KVM kernel module provides the ability to utilize the hardware-accelerated virtualization features provided by the nost CPU. QEMU-KVM also supports paravirtual access to hardware devices such as networks, disk drives and clocks. We reccomend that you utilize Ubuntu on your host machine. However, KVM should work on any Linux-based distribution. Learn more about KVM at https://www.linux-kvm.org/ .

Note: KVM only works if your CPU has hardware virtualization support – either Intel VT-x or AMD-V. To determine whether your CPU includes these features, run the following command:
```
$> egrep -c ‘(svm|vmx)’ /proc/cpuinfo
```
A 0 indicates that your CPU doesn’t support hardware virtualization, while a 1 or more indicates that it does. You may still have to enable hardware virtualization support in your computer’s BIOS, even if this command returns a 1 or more

Use the following command to install the dependencies necessary for installing QEMU-KVM:
```
$> sudo apt-get install git libglib2.0-dev libfdt-dev libpixman-1-dev zlib1g-dev flex bison
```
You can now install QEMU from source:
```
# Clone the QEMU repository
$> git clone git://git.qemu-project.org/qemu.git
# Initialize the DTC submodule (necessary to build QEMU-KVM)
$> git submodule update --init dtc
# Build DTC
$> cd dtc
$> make
# Build and configure QEMU for x86-64 machines
$> cd ..
$> mkdir bin
$> cd bin
$> ../configure --target-list=x86_64-softmmu --enable-debug
$> make
$> sudo make install
# Check if QEMU works
$> qemu-system-x86_64 --version
```
We reccomend you use a version of QEMU > 2.5. The instructions have been tested to work on QEMU 2.8

Install supporting packages such as `qemu-kvm`, `libvirtd` and `virt-manager`. Virt-Manager is a graphical application for managing your virtual machines — you can use the kvm command directly, but libvirt and Virt-Manager simplify the process.
```
$> sudo apt-get install qemu-kvm libvirt-bin bridge-utils virt-manager
```

Perform the following steps to ensure `libvirtd` and `kvm` use the latest versions of `QEMU`:
```
$> sudo rm /usr/bin/qemu*
$> sudo cp /usr/local/bin/qemu-* /usr/bin
```

Check that the version of QEMU used by KVM is same as the version of QEMU installed from source:
```
$> kvm --version
```

Only the root user and users in the libvirtd group have permission to use KVM virtual machines. Run the following command to add your user account to the libvirtd group:
```
$> sudo adduser <username> libvirtd
# Restart the daemon 
$> sudo /etc/init.d/libvirt-bin restart
```

After running this command, log out and log back in. Run this command after logging back in and you should see an empty list of virtual machines. This indicates that everything is working correctly.
```
$> virsh -c qemu:///system list
```

### Step 2: Create a Debian-based VM ###
Once you’ve got KVM installed, the easiest way to use it is with the Virtual Machine Manager application, which provides a GUI to do so. You’ll find it in your Dash (in Ubuntu, for other Distributions use the appropriate "start-button" like feature). Search for Virtual Machine Manager.Alternately, the Virtual Machine Manager can be launched from a terminal by:
```
$> virt-manager
```
Once the GUI appears, click the Create New Virtual Machine button on the toolbar and the Virtual Machine Manager will walk you through selecting an installation method, configuring your virtual machine’s virtual hardware, and installing your guest operating system of choice. Follow the steps required to setup the machine:

1. Download an ISO image of your favourite Debian-based distribution on your machine (preferred Ubuntu 14.04, as the setup instructions have been tested). 
2. Use the image to install the VM using `virt-manager` to confiure the CPUs, RAM, disk space, network etc. of your VM using the GUI (Note: the same can also be done using an XML config file).
3. Boot and Install the VM.
4. Once installed, upgrade the kernel to your preferred version (v4.1 + preferred). (Instructions to do so can be found here https://askubuntu.com/questions/119080/how-to-update-kernel-to-the-latest-mainline-version-without-any-distro-upgrade)
5. Install the necessary system applications (perform this step inside the VM)
```
$> sudo apt-get install build-essential git cmake cmake-curses-gui gawk flex bison perl doxygen u-boot-tools 
$> sudo apt-get install nfs-kernel-server tftpd-hpa isc-dhcp-server ufw
```
You should now have a working virtual machine with a debian-based distribution and Linux kernel version of choice. To utilize the paravirtual extensions provided by the QoT Stack further configuration is required. Please check out the Virtualization sub section in the section on building and installing the QoT Stack

# QoT Stack Build Instructions #

Assuming that you have successfully built and net-booted the kernel and a debian-based distribution (or installed the kernel on an SD card), you can now setup your host environment to cross-compile kernel modules and applications for the BeagleBone. The same instructions are applicable to the x86 setup or setup inside a virtual machine as well.

NOTE YOUR SETUP TYPE BEFORE GOING AHEAD:

1. FOR THE BBB SETUP, EVERYTHING IN THIS SECTION SHOULD BE CARRIED OUT ON THE HOST. IF YOU WANT, YOU CAN USE YOUR CONTROLLER TO BE THE HOST AS WELL (In this case, jump to Step 2). 
2. For the BBB SD card-based setup skip to STEP 2 of the Build Instructions
3. FOR x86 MACHINES, THE MACHINE ITSELF CAN BE THE HOST/CONTROLLER (In this case, jump to Step 2 as well).
4. FOR A VIRTUAL MACHINE with paravirtual support the stack needs to be installed on the host as well as the VM. (In this case, jump to Step 2 as well, and follow all the steps on both the VM and the host).
5. FOR A VIRTUAL MACHINE without paravirtual support the stack needs to be installed only on the VM. (In this case, jump to Step 2 as well)

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

Then, install an overlay for the device tree compiler. This compiles a new version of `dtc` which you can use to build overlays for the BeagleBone Black. Skip if you are setting up an x86 test bed or a VM-based setup.

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

The script will ask you to select a build type (depending on the target architecture). For the ARM-based BBB target choose `armv7l.linux-dev`. For an x86-based target choose `x86_64.linux-dev`. Assuming that you chose the build type to be `armv7l.linux-dev`, then you will see that a new script `envs-armv7l.linux-dev.sh` was created in the root of the OpenSplice directory. Similarly, for an x86-based target (or VM on an x86 machine) you will see that a new script `envs-x86_64.linux-dev.sh` was created in the root of the OpenSplice directory. You need to first source that script and then build. The build products will be put in the ./install directory. 

For an ARM-based BBB:
```
$> . envs-armv7l.linux-dev.sh
```
For an x86-based machines (or VM on an x86 host):
```
$> . envs-x86_64.linux-dev.sh
```
Make and install Opensplice
```
$> make
$> make install
```

Then, add `C++ 11` support by inserting the `-std=c++11` argument to the `CPPFLAGS` variable in `./install/HDE/%build%/custom_lib/Makefile.Build_DCPS_ISO_Cpp_Lib` file. Note that replace `%build%` with `envs-armv7l.linux-dev` or `envs-x86_64.linux-dev`, based on the target arhitecture.

You will now need to recompile the C++ interface:
```
$> pushd install/HDE/%build%/custom_lib
$> make -f Makefile.Build_DCPS_ISO_Cpp_Lib
$> popd
$> popd
```

You now have a working OpenSplice distribution with C++11 support. This basically provides a fully-distributed publish-subscribe messaging middleware with quality of service support. This mechanism will be used to advertise timelines across the network. We now elaborate the process to install opensplice

### (i) For the BBB 
The slaves (NFS-based or SD card-based) have their own arm versions of Java, ROS and OpenSplice 6.4 in the `/opt` directory of the rootfs, and whenever you SSH into a slave the `/etc/profile` script initializes all three for you. 

Note that whenever you run an OpenSplice-driven app you will need to set an environment variable `OSPL_URI` that configures the domain for IPC communication. This is described by an XML file, which is usually placed somewhere in your OpenSplice source tree. There are some default files. For the NFS-based BBB setup, the slave rootfs is configured by default to find the XML configuration in /mnt/openxplice/ospl.xml, as the configuration needs to be different for each slave -- they have different IPs and thus different`<NetworkInterfaceAddress>` tag values. You may already have executed the instructions for installing and configuring this XML file for unique BBB IP address in Step 6 of Controller Preparation above.

### (ii) For x86-based machines 
Follow the following steps:
```
$> cd /opt
$> mkdir opensplice
```
Navigate to the `qot-stack` directory
```
$> cd thirdparty/opensplice/install/HDE
$> sudo cp -R x86_64.linux-dev /opt/opensplice
```
Note that whenever you run an OpenSplice-driven app you will need to set an environment variable `OSPL_URI` that configures the domain for IPC communication. This is described by an XML file, which is usually placed somewhere in your OpenSplice source tree. There are some default files, mostly `ospl.xml`. As most of your applications will run in super user mode, add the following line to your bashrc (super user bashrc) `source /opt/opensplice/release.com`
```
$> sudo su
$> vim ~/.bashrc
```
Once you re-login to the shell (or re-run bashrc) you should now have a working opensplice distribution with all the environment variables set. 

### (iii) For x86-based virtual machines  
Follow the following steps on the virtual machine (VM) as well as the VM host machine:
```
$> cd /opt
$> mkdir opensplice
```
Navigate to the `qot-stack` directory
```
$> cd thirdparty/opensplice/install/HDE
$> sudo cp -R x86_64.linux-dev /opt/opensplice
```
Note that whenever you run an OpenSplice-driven app you will need to set an environment variable `OSPL_URI` that configures the domain for IPC communication. This is described by an XML file, which is usually placed somewhere in your OpenSplice source tree. There are some default files, mostly `ospl.xml`. As most of your applications will run in super user mode, add the following line to your bashrc (super user bashrc) `source /opt/opensplice/release.com`
```
$> sudo su
$> vim ~/.bashrc
```
Once you re-login to the shell (or re-run bashrc) you should now have a working opensplice distribution with all the environment variables set. 

## STEP 5 : Build configuration and install ##

Before building, make sure that you have followed the steps [here](https://bitbucket.org/rose-line/qot-stack/src/e856dbca2ddb1ecc83c0a14b743aa69d4b30bd93/src/test/?at=thorn16_refactor) for unit tests.

The entire project is cmake-driven, and so the following should suffice:

```
$> mkdir -p build % Do this in the top most project directory /qot-stack %
$> pushd build
$> ccmake ..
```
You should now see a cmake screen in your terminal with multiple configuration options. The configuration options you choose will depend on the target build. The different configuration options are elaborated below:

### (i) Build configuration options 
The following configuration options can be configured in the QoT Stack:

#### (a) Library and Installation Path Configuration Options 
**Note**: These options should be set automatically, change only if you have problems while configuring the options. If you are missing some dependencies (such as Boost), you may need to manually install the missing dependencies.
```
1. BOOST_THREAD_LIBRARY          - Path to the Boost Library (Boost 1.55 Required and must be installed)
2. CMAKE_INSTALL_PREFIX          - Prefix of the installation path   
3. CMAKE_BUILD_TYPE              - Undefined (not used for now)
4. DCPSCPP_LIBRARY               - Location of the opensplice C++ Library
5. DCPSISOCPP_LIBRARY            - Location of the opensplice ISO C++ Library
6. KERNEL_LIBRARY                - Location of the opensplice kernel Library
7. OpenSplice_INCLUDE_DIR        - Location of the opensplice headers
```
#### (b) Component configuration 
**Note**: These options decde which components get compiled and installed to the target.
```
1. BUILD_CPP_API                    - Build the C++ API and Libraries                                    
2. BUILD_CPP_EXAMPLE                - Build the C++ examples (C++ API and lib must also be built)       
3. BUILD_PROGAPI                    - Build the C API and Libraries   
4. BUILD_EXAMPLE                    - Build the C examples (C API and lib must also be built)       
5. BUILD_GPS_SERVICE                - Build GPS-PPS Synchronization Module        
6. BUILD_MODULES                    - Build Core QoT Kernel Module (Necessary for QoT Stack operation)  
7. BUILD_QOTLATD                    - Build the userspace daemon which estimates clock-read latencies   
8. BUILD_SERVICE                    - Build the PTP/NTP-based QoT-enabled synchronization service        
9. BUILD_UNITEST                    - Build math unit tests                                              
10. BUILD_VIRT                      - Build Virtualization components for a host machine     
11. BUILD_VIRT_GUEST                - Build Virtualization components for a para-virtual guest machine  
12. CROSS_AM335X                    - Cross-compile using an ARM compiler for the Beaglebone Black 
13. X86_64                          - Compile the stack using the Native x86 compilers
14. GENERIC_BUILD                   - Generic software-based clock driver (x86 host, non-PV guest, BBB ..)
```
**Important Configuration Notes**:

1. Component Configuration
  - Options 1 to 9 (in the list) dictate which modules will be built. We reccomend that you build all. 
  - Options 6, 7 and 8 are essential to compile the kernel modules and system services and are key to running all QoT stack functionality.
  - Options 1 and 2 are needed to build the C++ QoT application libraries and the C++ example applications.
  - Options 3 and 4 are needed to build the C QoT application libraries and the C example applications
2. For the BBB: 
  - make sure that CROSS_AM335x is ON and that your INSTALL_PREFIX is `/export/rootfs/usr/local`. This will build the dedicated QoT clock driver for the QoT Stack and ensure that the proper ARM cross compiler is utilized for compilation.
  - set GENERIC_BUILD and CROSS_AM335x to ON if you want a generic software QoT clock driver on the Beaglebone Black (not reccomended)
3. For the x86 platform (native or virtualization host/guest) set X86_64 to ON. Depending on the machine type (virtual/native), please turn on the additional configuration options:
  - For an x86 host, which will host Virtual Machines with paravirtual support set BUILD_VIRT to ON.
  - For x86-based Virtual Machines (guest) with paravirtual support set BUILD_VIRT_GUEST to ON.
   
Once the options are configures press `c` to generate the configuration. If the configuration is succesful, press `g` to generate the makefiles and exit the CMake Configuration screen. You are now ready to build the stack.

### (ii) Build and Install 
```
$> make
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

Now, install the stack using the following commands.
```
$> sudo make install
$> popd
```
This will install the QoT stack binaries to the path specified in `CMAKE_INSTALL_PREFIX`. We reccomend that you add this path to the `PATH` environment variable on your machine. For the NFS-based developmnt setup, the installation will happen to  the NFS share located at `/export/rootfs`. This should make the binaries be available instantly on the BBB nodes. 

After installing the user-space applications you might need to ssh into the nodes (for the BBB) and run `ldconfig` on the node. SImilarly run `ldconfig` on your taret x86 machine or VM if you are setting them up as your target.

## STEP 6: Install kernel modules ##

All the commands specified in this section need to be run in the top most project directory (\qot-stack). We utilize the Makefile in this top directory to install the kernel modules. Before following any of the steps in this section, please open the Makefile and edit the `KERNELVER` variable to the kernel version of your target device. This can be found by running `uname -a` on your target device.

For the BBB-based setup run the following to build the device tree overlay required for configuring the pins on the AM335x. Skip this step for other setups.
```
$> make
```

For the NFS-based BBB setup, to install the kernel modules run the following. Skip for other setups:
```
$> sudo make install
```

For the SD card-based BBB setup, to install the kernel modules run the following. Skip for other setups:
(i) Replace the field IPADDR with the ip-address of the node (obtained from the previous step) 
(ii) do  a `make install_sd`. All the latest versions of the stack componenets and libraries are now on the node.

For the x86-based machines (virtual and native), to install the kernel modules run the following. Skip for other setups:
```
$> sudo make install_x86
```

After installing the kernel modules you might need to ssh into the nodes and run `depmod -a` on the nodes to build the dependencies between the different kernel modules.

# Initializing the QoT stack #
Please choose the subsection based on the target platform you are utilizing.

## 1. On the BBB (both NFS-based and SD card setups) ##
Firstly, SSH into a node of choice:
If you type `capes` on the command line you should see four slots as empty:

```
root@arm:~# capes
 0: PF----  -1 
 1: PF----  -1 
 2: PF----  -1 
 3: PF----  -1 
```

When you installed the kernel module a DTBO file was copied to  `/export/rootfs/lib/firmware`. This is a device tree overlay file that tells the BeagleBone how to multiplex its I/O pins, and which kernel module to load after it has done so. To apply the overlay use the `capes` command. Before using capes use depmod and ldconfig to ensure that the libraries and kernel module dependencies are resolved.

```
$> depmod -a
$> ldconfig
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

If these commands, give you path error, do the following, (replace `KERNELVER` with the value of the KERNELVER variable used in the Makefile in the previous section (kernel module installation) 

```
root@arm:~# cd /lib/modules/<KERNELVER>/kernel/drivers/misc
root@arm:~# insmod qot.ko
root@arm:~# insmod qot_am335x.ko
```

If you still have issues, do "dmesg" and see the logs for issues.

## 2. On Native x86 Platforms ##
The QoT stack kernel modules can be loaded by the following commands:
```
$> depmod -a
$> ldconfig
$> modprobe qot_x86
```
The modprobe command should automatically load both the QoT module as well as the x86 QoT clock driver. If it succeeds, running the `lsmod` command should list two modules `qot` and `qot_x86`. If this does not work you can use `insmod` to load both the kernel modules manually from the path to contains their respective .ko files. See the Makefile (used in the previous section) for the path to where the kernel module binaries are installed. 

## 3. On x86-based Virtual Machines (without Paravirtual Support)
The QoT stack kernel modules can be loaded by the following commands:
```
$> depmod -a
$> ldconfig
$> modprobe qot_x86.ko
```
The modprobe command should automatically load both the QoT module as well as the x86 QoT clock driver. If it succeeds, running the `lsmod` command should list two modules `qot` and `qot_x86`. If this does not work you can use `insmod` to load both the kernel modules manually from the path to contains their respective .ko files. See the Makefile (used in the previous section) for the path to where the kernel module binaries are installed. 

## 4. On x86-based Virtual Machines (with Paravirtual Support)
### STEP 1: Configure the Host ###
The QoT stack kernel modules can be loaded by the following commands:
```
$> depmod -a
$> ldconfig
$> modprobe qot_x86.ko
```
The modprobe command should automatically load both the QoT module as well as the x86 QoT clock driver. If it succeeds, running the `lsmod` command should list two modules `qot` and `qot_x86`. If this does not work you can use `insmod` to load both the kernel modules manually from the path to contains their respective .ko files. See the Makefile (used in the previous section) for the path to where the kernel module binaries are installed. 

Now run the `ivshmem server` which provides the shared memory space used to transfer QoT information and timeline mappings from the host to the guests (this needs to be checked)
```
$> ivshmem-server -F -v
```
NOTE: If the above command fails, the following command may have to be executed:
```
$> rm /tmp/ivshmem_socket
```
This needs to be done to delete any stale UNIX domain socket that might be present prior to `ivshmem-server` execution.

Next, run the QoT Virtualization Daemon, which enables Paravirtual Guest VMs to access the QoT functionality (clock sync, timeline management and QoT estimation) provided by the host:
```
$> qot_virtd
```
If the above command fails, pleasue ensure the QoT Stack installation path is there in the `PATH` environment variable. 

Note: `qot_virtd` acts as a server and uses `ivshmem` and `VirtIO Serial` included in QEMU-KVM to provide this paravirtual functionality. The different quest VMs are clients of the `qot_virtd` server. Timeline specific API calls are transported from clients to servers using `VirtIO Serial`, while clock sync and uncertainty imformation are transported from server to client using shared memory exposed by `ivshmem`. Check out this link to know more about using `ivshmem` http://nairobi-embedded.org/linux_pci_device_driver.html

### STEP 2: Launch and Configure the Guest ###
To utilize the paravirtual functionality exposed by the `qot_virtd` server on the host, you need to configure the VM. We assume you have a VM with the QoT stack installed and prepared, prior to these steps. 

You now need to add some command line configurations, while launching the VM. Passing command line option to qemu from virt-manager requires the following steps:
```
$> virsh edit <name of vm> 
```
This will bring up an XML file which you can edit. ALternately, you can directly modify the file using vim /etc/libvirt/qemu/<name of virtual machine>.xml

Now, in the file, change <domain type='kvm'> to <domain type='kvm' xmlns:qemu='http://libvirt.org/schemas/domain/qemu/1.0'>

Next, we need to add the following XML tag block inside the `<domain>` block (prefereably at the end before the `</domain>) for the required command line parameters. 
```
<qemu:commandline>
    <qemu:arg value='-chardev'/>
    <qemu:arg value='socket,path=/tmp/ivshmem_socket,id=ivshmemid'/>
    <qemu:arg value='-device'/>
    <qemu:arg value='ivshmem,chardev=ivshmemid,size=4,ioeventfd=on'/>
    <qemu:arg value='-chardev'/>
    <qemu:arg value='socket,path=/tmp/qot_virthost,id=qot_virthost'/>
    <qemu:arg value='-device'/>
    <qemu:arg value='virtio-serial'/>
    <qemu:arg value='-device'/>
    <qemu:arg value='virtserialport,chardev=qot_virthost,name=qot_virthost'/>
</qemu:commandline>
```
These command line parameters are necessary to configure the VM to access the read-only shared memory and the VirtIO-Serial socket required for communication with the QoT Stack components on the host.

You can now boot up your VM using the `virt-manager` GUI.

**Note**: To succesfully boot your VM you should have the daemons `qotvirtd` and `ivshmem-server` running on the host.

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

This code is maintained by University of California Los Angeles (UCLA) and Carnegie Mellon University (CMU) on behalf of the Roseline project. If you wish to contribute to development, please fork the repository, submit your changes to a new branch, and submit the updated code by means of a pull request against origin/thorn16_refactor.

# Support #

Questions can be directed to Fatima Anwar (fatimanwar@ucla.edu).