# Overview #

This project is intended for developers, and so it presumes a certain working knowledge of embedded Linux. The general idea is to have BeagleBones fetch a Linux kernel and device tree over TFTP from a controller, and then mount an NFS share at the root file system. In this was we don't have to insert and eject many microsd cards, and we are guaranteed to have a consistent version of firmware across all nodes.

1. Controller (Ubuntu 15.04, x86_64-linux-gnu) - NAT router, DCHP server, NFS server, TFTP server
1. Host (Ubuntu 15.04, x86_64-linux-gnu) - Where you do your development
1. Slave (Ubuntu 15.04, arm-linux-gnueabihf) - The actual BeagleBones

Since the synchronization algoritnm is based on wired PTP, for the qot-stack to work effectively you will need a IEEE 1588v2 compliant network. The slaves have a PTP-compliant Ethernet adapter, and Linux supports hardware timestamping out of the box. Our version of PTP is derived from the linuxptp project.

Robert Nelson's ```bb-kernel``` project provides almost everythign we need to build a suitable kernel for the BeagleBone Black. The idea will be to check this project out on the controller and build a kernel for the slaves. We will export this project alogn with the rootfs over NFS. The reason for this is that we can NFS-mount it at /export on our host environment and cross-compile and install kernel modules and user-space applications very easily.

So, to summarize, you will end up having this on directory (/export) on your central controller:

```
- /export
  - bb-kernel : kernel build script
  - rootfs : location of the root file syste,
  - tftp : contains kernel image and device trees
```

And, you will NFS mount this on each host. When you compile the kernel you should use the cross-compiler that is automatically downloaded by the kernel build script. However, when you compile applications you need to use the ```arm-linux-gnueabihf``` compiler in the Ubuntu. Also, it is really important that the version of this GCC compiler matches the version deployed on the slaves (currently 4.9.2). The reason for this is that different compilers have different libc versions, which causes linker errors that are very tricky to solve.

# Controller preparation #

This section describes how to prepare your central controller. However, in order to do it must make some assumptionsm about your controller. In order to be used as a NAT router, your controller must have at least two interfaces. I'm going to assume the existence of these two adapters, and you will need to modify the instructions if they are different. You can use the network configuration manager in Ubuntu to configure them accordingly.

1. eth0 - connected through a LAN to the slaves (static IP 10.42.0.1 and only local traffic)
2. wlan0 - connected to the internet (address from)

Install the necessary system applications

```
sudo apt-get install nfs-kernel-server tftpd-hpa isc-dhcp-server ufw
```

EVERYTHING IN THIS SECTION MUST BE EXECUTED ON THE CONTROLLER.

## STEP 1 : Install the root filesystem and kernel  ##

First create two important directories:

```
$> sudo mkdir -p /export/rootfs
$> sudo mkdir -p /export/tftp
```

Then, download the rootfs and decompress it to the correct location

```
$> wget -O qotrootfs.tar.bz2  https://tinurl.com/qotrootfs
$> sudo tar -xjpf qotrootfs.tar.bz2 -C /export/rootfs
```

Finally, checkout, build and install the kernel:

```
$> su -
$> cd /export
$> git clone https://bitbucket.com/rose-line/bb-kernel.git
$> cd bb-kernel
$> ./build_kernel.sh
$> ./install_netboot.sh
```

## STEP 2 : Configure DHCP  ##

The Ubuntu host needs to act as a DHCP server, assigning IPs to slaves as they boot. I personally prefer to define each of my slaves in the configuration file so that they are assigned a constant network address that is preserved across booting. Edit the ```/etc/default/isc-dhcp-server``` file to add the interface on which you wish to serve DHCP requests:

```
INTERFACES="eth0"
```

Now, configure the DHCP server and add an entry for each slave node in ```/etc/dhcp/dhcpd.conf``` . The configuration below assigns the IP ```10.42.0.100``` to the single slave with an Ethernet MAC ```6c:ec:eb:ad:a7:3c```. To add more slaves, just add additional lines to this file.

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

# STEP 3 : Configure NAT #

In the file ```/etc/default/ufw``` change the parameter ```DEFAULT_FORWARD_POLICY```

```
DEFAULT_FORWARD_POLICY="ACCEPT"
```

Then, configure ```/etc/ufw/sysctl.conf``` to allow ipv4 forwarding.

```
net.ipv4.ip_forward=1
#net/ipv6/conf/default/forwarding=1
#net/ipv6/conf/all/forwarding=1
```

Then, add the following to ```/etc/ufw/before.rules``` just before the filter rules (starts with ```*filter```).

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

You'll also need to add a rule that allows all traffic in on ```eth0``` so that the slaves can access services.


```
$> sudo ufw allow in on eth0
```

## STEP 4 : Configure TFTP  ##

Then, edit the ```/etc/default/tftpd-hpa``` file to the following:

```
TFTP_USERNAME="tftp" 
TFTP_DIRECTORY="/export/tftp" 
TFTP_ADDRESS="10.42.0.1:69" 
TFTP_OPTIONS="-s -c -l"
```

Then, restart the server:

```
sudo service tftpd-hpa restart
```

## STEP 5 : Configure NFS  ##

Edit the NFS ```/etc/exports``` on the share the ```/export``` directory 

```
/export 10.42.0.0/24(rw,sync,no_root_squash,no_subtree_check)
```

Then, restart the server:

```
$> sudo service nfs-kernel-server restart
```

## STEP 6 : Netboot a test slave  ##

To boot the qot stack you'll need a well-formed microsd card. Please note that fdisk is a partitioning tool, and if you do not specify the correct device, you run the risk of formatting the incorrect drive. So, please use ```tail -f /var/log/syslog``` before inserting the card to see which device handle (/dev/sd?) is assigned by udev when the card is inserted into your PC.

Then run the following:

1. Start fdisk to partition the SD card: ```sudo fdisk /dev/sd?```
1. Type ```o```. This will clear out any partitions on the drive.
1. Type ```p``` to list partitions. There should be no partitions left.
1. Type ```n```, then ```p``` for primary, ```1``` for the first partition on the drive, ```enter``` to accept the default first sector, then ```+64M``` to set the size to 64MB.
1. Type ```t``` to change the partition type, then ```e``` to set it to W95 FAT16 (LBA).
1. Type ```a```, then ```1``` to set the bootable flag on the first partition.
1. Type ```n```, then ```p``` for primary, ```2``` for the second partition on the drive, and ```+2GB``` to set the size to 2GB.
1. Type ```n```, then ```p``` for primary, ```3``` for the third partition on the drive, and ```enter``` to select the remaining bytes on the drive.
1. Write the partition table and exit by typing ```w```.
1. Initialise the filesystem for partition 1: ```mkfs.vfat -F 16 /dev/sd?1 -l boot```
1. Initialise the filesystem for partition 2: ```mkfs.ext4 /dev/sd?2 -l rootfs```
1. Initialise the filesystem for partition 3: ```mkfs.ext4 /dev/sd?3 -l user```

Now, install the bootloader:

```
$> su -
$> sudo mount /dev/sd?1
$> cd /mnt
$> wget -O MLO http://tinyurl.com/MLO
$> wget -O u-boot.img http://tinyurl.com/u-boot.img
$> wget -O uEnv.txt http://tinyurl.com/uEnv.txt
$> cd /
$> umount /mnt
```

Using this card you you should be able to net-boot a Rev C BeagleBone that is plugged into the same LAN as the controller. The four blue LEDs will light up sequentially until all are lit, and you should see the green LED on the Ethernet jack flicker as the kernel / device tree are downloaded from the controller. All four blue LEDS will then turn off for a few seconds while the kernel begins the boot process. They should then rapidly flick for a few more seconds before the classic heartbeat led begins. 

To determine the IP address that the controller offered, or debug any errors check the system log:

```
$> tail -f /var/log/syslog
```

If you can't find anything useful there, use a FTDI cable to inspect the u-boot process of the slave.


# Build instructions #

Now that we have a working kernel


## Install system dependencies, checkout code and initialize submodules ##

```
$> sudo apt-get install gawk flex bison perl doxygen u-boot-tools 
```

Configure your system with a workaround for GCC 4.9.2 and Ubuntu 15.04:

```
$> sudo mkdir -p /usr/lib/bfd-plugins
$> sudo ln -s /usr/lib/gcc/x86_64-linux-gnu/4.9.2/liblto_plugin.so /usr/lib/bfd-plugins/
```

Check out the qot-stack code

```
$> git clone https://bitbucket.org/asymingt/qot-stack.git
$> cd qot-stack
```

Fetch third party code

```
$> git submodule init
$> git submodule update
```

This will take a while, as it checks out several large third party repos.

Now, install and reload our udev rules.

```
$> sudo cp src/module/80-qot.rules /etc/udev/rules.d/
$> sudo service udev restart
```

These rules force the ```/dev/qot``` and ```/dev/timelineX``` to RW permission for all users.

## Build the kernel ##

First, build a vanilla kernel...

```
$> pushd thirdparty/bb-kernel
$> git checkout 4.1.3-bone15 -b v4.1.3
$> ./build_kernel.sh

```

This will take a long time, as it will compile an entire Linux-based system.

Now, replace the KERNEL directory with a symbolic link to the Kronux kernel.

```
$> mv KERNEL KERNEL-DEFAULT
$> ln -s ../Kronux KERNEL
```

Then, copy the kernel defconfig for the BeagleBoneBlack and rebuild

```
$> pushd ../Kronux
$> cp arch/arm/configs/roseline_defconfig .config
$> popd
```

Finally, rebuild the kernel

```
$> ./tools/rebuild_kernel.sh
```

## Build the service ##

Switch to the OpenSplice directory and pull the third party repo for the C++ bindings

```
$> pushd thirdparty/opensplice
$> git submodule init
$> git submodule update
```

Configure and build the OpenSplice DDS library. The configure script searches for third party dependencies. The third party libraries ACE and TAO are only required for Corba, and in my experience introduce compilation errors. So, I would advise that you do not install them. 

```
$> ./configure
```

Assuming that you chose the build type to be x86_64.linux-dev, then you will see that a new script ```envs-x86_64.linux-dev.sh``` was created in the root of the OpenSplice directory. You need to first source that script and then build. The build products will be put in the ./install directory. It's probably wise to not move them, as the qot-service expects them to be there.

```
$> . envs-x86_64.linux-dev.sh
$> make
$> make install
$> popd
```

Add c++11 support, by inserting the ```-std=c++11``` argument to the ```CPPFLAGS``` variable in ```install/HDE/x86_64.linux-dev/custom_lib/Makefile.Build_DCPS_ISO_Cpp_Lib``` file. You will then need to recompile the C++ interface:

```
$> pushd thirdparty/opensplice/install/HDE/%build%/custom_lib
$> make -f Makefile.Build_DCPS_ISO_Cpp_Lib
$> popd
```

You now have a working OpenSplice distribution with C++11 support. This basically provides a fully-distributed publish-subscribe messaging middleware with quality of service support. This mechanism will be used to advertise timelines across the network.

Note that whenever you run an OpenSplice-driven app you will need to set an environment variable ```OSPL_URI``` that configures the domain for IPC communication. This is described by an XML file, which is usually placed somewhere in your OpenSplice source tree. There are some default files.

There are some good C++ examples showing how to use OpenSplice DDS at ```https://github.com/PrismTech/dds-tutorial-cpp-ex```. When compiled, the ch1 example produces a tssub and tspub application. To ensure that these applications know how to configure the domain they use, they must be run in the following way:

```
OSPL_URI=file:///path/to/opensplice/etc/ospl.xml ./tspub 1
```

Note that on devices with multiple network interface cards (eg. wlan0, eth0, etc.) you must explicitly state the IP address of the interface in ```<NetworkInterfaceAddress></NetworkInterfaceAddress>``` XML tag.

Finally, build the qotdaemon application

```
$> mkdir -p build
$> cd build
$> cmake ..
$> make
```


## NOTES ##

There are three 

1. PTP-compliant NIC 			Performs filtered H/W timestamping

   a. With CLK
   b. With SFD
   c. With CLK and SFD	
   d. Without CLK and SFD

2. Regular NIC 					Does not perform H/W timestamping

   a. With CLK
   b. With SFD
   c. With CLK and SFD			
   d. Without CLK and SFD

IMPORTANT

All PTP-compliant devices have an RX filter to determine which packets
are timestamped. This can vary based on the manufacturer, so no method
is guaranteed. For example, the BBB Ethernet adapter stamps 802.3 L2
packets only. Others may also stamp L4 IPv4 or IPv6 UDP packets. For 
this reason our PTP client must support all options.

In the first instance all our QoT stack will adapt is the location of
the master, and the node synchronization frequency. In future we will
support switching between NICs, sync alorithms and adaptive clocking.