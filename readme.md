## INSTALLATION INSTRUCTIONS ##

This project is intended for developers, and so it presumes a certain working knowledge of embedded Linux. The general idea is to have BeagleBones fetch a Linux kernel and device tree over TFTP from a controller, and then mount an NFS share at the root file system. In this was we don't have to insert and eject many microsd cards, and we are guaranteed to have a consistent version of firmware across all nodes.

1. Controller (x86_64-linux-gnu) - controls the test bed 
1. Host (x86_64-linux-gnu) - where you do your development
1. Slave (arm-linux-gnueabihf) - the BeagleBones

Since the synchronization the qot-stack to work effectively, you will need a IEEE 1588v2 compliant network .


# Build instructions #

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