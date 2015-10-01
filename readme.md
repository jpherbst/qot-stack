1. You'll need to initialize the OpenDDS submodule in thirdparty
2. You'll need to ```sudo apt-get install build-essential cmake```
3. Build instructions
```
mkdir build
pushd build
cmake ../src
make -j4
popd
4. Running instructions
```
pushd build
./qotdaemon -h
```

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
