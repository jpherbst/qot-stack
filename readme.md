## INSTALLATION INSTRUCTIONS ##

# Install opensplice #

Configure your system with a workaround for Ubuntu 15.04:

```
$> sudo mkdir -p /usr/lib/bfd-plugins
$> sudo ln -s /usr/lib/gcc/x86_64-linux-gnu/4.9.2/liblto_plugin.so /usr/lib/bfd-plugins/
$> sudo apt-get install gawk flex bison perl
```

Check out opensplice to /opt/opensplice

```
$> cd thirdparty
$> git clone https://github.com/PrismTech/opensplice.git
$> cd opensplice
$> git checkout OSPL_V6_4_OSS_RELEASE -b V6_4
```
Compile opensplice

```
$> ./configure
$> make
$> make install
```

Add c++11 support, by inserting the ```-std=c++11``` argument to the ```CPPFLAGS``` variable in ```install/HDE/x86_64.linux-dev/custom_lib/Makefile.Build_DCPS_ISO_Cpp_Lib``` file. You will then need to recompile the C++ interface:

```
cd install/HDE/x86_64.linux-dev/custom_lib
make -f Makefile.Build_DCPS_ISO_Cpp_Lib
```

You now have a working OpenSplice distribution with C++11 support. This basically provides a fully-distributed publish-subscribe messaging middleware with quality of service support. This mechanism will be used to advertise timelines across the network.

Note that whenever you run an OpenSplice-driven app you will need to specify an environment variable that tells the library about the domain over which communication will take place. This is described in an XML file, which is usually placed somewhere in your OpenSplice source tree. 

There are some good C++ examples showing how to use OpenSplice DDS at ```https://github.com/PrismTech/dds-tutorial-cpp-ex```. When compiled, the ch1 example produces a tssub and tspub application. To ensure that these applications know how to configure the domain they use, they must be run in the following way:

```
OSPL_URI=file:///path/to/opensplice/etc/ospl_sp_no_network.xml ./tspub 1
```

# Build the daemon #

Whene

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
