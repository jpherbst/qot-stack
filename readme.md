## INSTALLATION INSTRUCTIONS ##

1. Install opensplice 

Configure your system with a workaround for Ubuntu 15.04:

```
$> sudo mkdir -p /usr/lib/bfd-plugins
$> sudo ln -s /usr/lib/gcc/x86_64-linux-gnu/4.9.2/liblto_plugin.so /usr/lib/bfd-plugins/
$> sudo apt-get install gawk flex bison perl
```

Compile opensplice to /opt/opensplice

```
$> pushd thirdparty/opensplice
$> git clone https://github.com/osrf/opensplice.git
$> cd opensplice/build
$> cmake .. -DCMAKE_INSTALL_PREFIX=/opt/opensplice
$> export CMAKE_PREFIX_PATH=/opt/opensplice:$CMAKE_PREFIX_PATH
$> sudo make install
$> popd
```

Build the daemon


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
