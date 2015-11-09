#!/bin/bash

# Parameters
ROOTFS=/export/rootfs
TFTPFS=/export/tftp
NETWAN=wlan0
NETLAN=eth0
BEAGLE=10.42.0.100

# Check out the submodules needed for QoT stack
git submodule init
git submodule update thirdparty/bb-kernel
git submodule update thirdparty/bb.org-overlays

# Pull changes to bb-kernel from upstream
pushd thirdparty/bb.org-overlays
git remote add upstream https://github.com/RobertCNelson/bb.org-overlays.git
git fetch upstream
./dtc-overlay.sh
popd

# Pull changes to bb-kernel from upstream
pushd thirdparty/bb-kernel
git remote add upstream https://github.com/RobertCNelson/bb-kernel.git
git fetch upstream
git checkout 4.1.12-bone-rt-r16
cp ../../extra/qot.config patches/defconfig
./build_kernel.sh
../../extra/install_netboot.sh
popd
