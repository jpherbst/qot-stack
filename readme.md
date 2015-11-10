# Table of contents #

[TOC]

# Overview #

This project is intended for developers, and so it presumes a certain working knowledge of embedded Linux. The general idea is to have BeagleBones fetch a Linux kernel and device tree over TFTP from a controller, and then mount an NFS share at the root file system. In this was we don't have to insert and eject many microsd cards, and we are guaranteed to have a consistent version of firmware across all nodes.

1. Controller (Ubuntu 15.04, x86_64-linux-gnu) - NAT router, DCHP server, NFS server, TFTP server
1. Host (Ubuntu 15.04, x86_64-linux-gnu) - Where you do your development
1. Slave (Ubuntu 15.04, arm-linux-gnueabihf) - The actual BeagleBones

Since the synchronization algorithm is based on wired PTP, for the qot-stack to work effectively you will need a IEEE 1588v2 compliant network. The slaves have a PTP-compliant Ethernet adapter, and Linux supports hardware time stamping out of the box. Our version of PTP is derived from the linuxptp project.

Robert Nelson's ```bb-kernel``` project provides almost everything we need to build a suitable kernel for the BeagleBone Black. The idea will be to check this project out on the controller and build a kernel for the slaves. We will export this project along with the rootfs over NFS. The reason for this is that we can NFS-mount it at /export on our host environment and cross-compile and install kernel modules and user-space applications very easily.

So, to summarize, you will end up having this on directory (/export) on your central controller:

```
- /export
  - bb-kernel : kernel build script
  - rootfs : location of the root file syste,
  - tftp : contains kernel image and device trees
```

And, you will NFS mount this on each host. When you compile the kernel you should use the cross-compiler that is automatically downloaded by the kernel build script. However, when you compile applications you need to use the ```arm-linux-gnueabihf``` compiler in the Ubuntu. Also, it is really important that the version of this GCC compiler matches the version deployed on the slaves (currently 4.9.2). The reason for this is that different compilers have different libc versions, which causes linker errors that are very tricky to solve.

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
Then, build the kernel using the QoT configuration. To do this, type the following command. When the Linux menu configuration appears, click the LOAD option at the bottom right and choose ```/export/qot.config``` as the path to the configuration file.

```
$> ./build_kernel.sh
```

Once the kernel has configured run the install script.

```
$> /export/install_netboot.sh
```

Now, you have a working kernel


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

## STEP 3 : Configure NAT ##

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
$> wget https://bitbucket.org/rose-line/qot-stack/downloads//MLO
$> wget https://bitbucket.org/rose-line/qot-stack/downloads/u-boot.img
$> wget https://bitbucket.org/rose-line/qot-stack/downloads/uEnv.txt
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

Assuming that you have successfully built and net-booted the kernel, you can now setup your host environment to cross-compile kernel modules and  applications for the BeagleBone.

EVERYTHING IN THIS SECTION SHOULD BE CARRIED OUT ON THE HOST

## STEP 1 : Setup the /export share ##

Recall that we NFS-exported the ```/export``` directory from the controller. Create the same directory on your host.

```
$> sudo mkdir -p /export
```

Now, we will add the following line to the end of the host's ```/etc/fstab``` file to automount this central controller's NFS share when the machine is booted.

```
10.42.0.1:/export /export nfs rsize=8192,wsize=8192,timeo=14,intr
```

Reboot the host and you should see the rootfs, tftp and bb-kernel directories in the ```/export``` directory.

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

Then, install an overlay for the device tree compiler. This compiles a new version of ```dtc``` which you can use to build overlays for the BeagleBone Black.

```
$> pushd thirdparty/bbb.org-overlays
$> ./dtc-overlay.sh
$> popd
```

## STEP 4 : Build OpenSplice ##

Switch to the OpenSplice directory and pull the third party repo for the C++ bindings

```
$> pushd thirdparty/opensplice
$> git checkout -b OSPL_V6_4_OSS_RELEASE v64
```

Configure and build the OpenSplice DDS library. The configure script searches for third party dependencies. The third party libraries ACE and TAO are only required for Corba, and in my experience introduce compilation errors. So, I would advise that you do not install them. 

```
$> ./configure
```

Assuming that you chose the build type to be x86_64.linux-dev, then you will see that a new script ```envs-x86_64.linux-dev.sh``` was created in the root of the OpenSplice directory. You need to first source that script and then build. The build products will be put in the ./install directory. 

```
$> . envs-x86_64.linux-dev.sh
$> make
$> make install
$> popd
```

Then, add C++11 support by inserting the ```-std=c++11``` argument to the ```CPPFLAGS``` variable in ```./install/HDE/x86_64.linux-dev/custom_lib/Makefile.Build_DCPS_ISO_Cpp_Lib``` file. You will now need to recompile the C++ interface:

```
$> pushd thirdparty/opensplice/install/HDE/%build%/custom_lib
$> make -f Makefile.Build_DCPS_ISO_Cpp_Lib
$> popd
```

You now have a working OpenSplice distribution with C++11 support. This basically provides a fully-distributed publish-subscribe messaging middleware with quality of service support. This mechanism will be used to advertise timelines across the network. The slaves have their own arm versions of Java, ROS and OpenSplice 6.4 in the ```/opt``` directory of the rootfs, and whenever you SSH into a slave the ```/etc/profile``` script initializes all three for you. 

Note that whenever you run an OpenSplice-driven app you will need to set an environment variable ```OSPL_URI``` that configures the domain for IPC communication. This is described by an XML file, which is usually placed somewhere in your OpenSplice source tree. There are some default files. The slave rootfs is configured by default to find the XML configuration in /mnt/openxplice/ospl.xml, as the configuration needs to be different for each slave -- they have different IPs and thus different```<NetworkInterfaceAddress>``` tag values.

## STEP 5 : Build and install the kernel module ##

The kernel modules are built and installed in the following way:

```
$> pushd module
$> ccmake ..
$> make
$> sudo make install
$> popd
```

After installing the kernel module you might need to run ```depmod``` on the node.

## STEP 6 : Build and install the user-space components ##

The entire project is cmake-driven, and so the following should suffice:

```
$> mkdir -p build
$> pushd build
$> ccmake ..
```

Make sure that CROSS_COMPILE is ON and that your INSTALL_PREFIX is ```/export/rootfs/usr/local```.

```
$> make
$> sudo make install
$> popd
```

This will install the QoT stack to your the NFS share located as ```/export/rootfs```. It should be available instantly on the nodes.

After installing the user-space applications you might need to run ```ldconfig``` on the node.

# Running the QoT stack #

Firstly, SSH into a node of choice:

If you type ```capes``` on the command line you should see four slots as empty:

```
root@arm:~# capes
 0: PF----  -1 
 1: PF----  -1 
 2: PF----  -1 
 3: PF----  -1 
```

When you installed the kernel module a DTBO file was copied to  ```/export/rootfs/lib/firmware```. This is a device tree overlay file that tells the BeagleBone how to multiplex its I/O pins, and which kernel module to load after it has done so. To apply the overlay use the ```capes``` command.

```
$> capes ROSELINE-QOT
```

The output of the ```capes``` command should now be this:

```
root@arm:~# capes
 0: PF----  -1 
 1: PF----  -1 
 2: PF----  -1 
 3: PF----  -1 
 4: P-O-L-   0 Override Board Name,00A0,Override Manuf,ROSELINE-QOT
```

And the ```lsmod``` command should list two new kernel modules:

```
root@arm:~# lsmod
Module                  Size  Used by
qot_am335x              7121  0 
qot_core                7655  3 qot_am335x
```

We have included a ```helloworld``` application to test the qot stack.


```
$> helloworld
```

Run this command on multiple nodes and the time should be synchronized transparently in the background.

## Development ##

Take a look at the ```src/examples``` for an idea of how to use the stack.
