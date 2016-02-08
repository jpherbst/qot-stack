# An End-to-end Quality of Time (QoT) Stack for Linux #

Having a shared sense of time is critical to distributed cyber-physical systems. Although time synchronization is a mature field of study, the manner in which Linux applications interact with time has remained largely the same -- synchronization runs as a best-effort background service that consumes resources independently of application demand, and applications are not aware of how accurate their current estimate of time actually is.

Motivated by the Internet of Things (IoT), in which application needs are regularly balanced with system resources, we advocate for a new way of thinking about how applications interact with time. We propose that Quality of Time (QoT) be both *observable* and *controllable*. A consequence of this being that time synchronization must now satisfy timing demands across a network, while minimizing the system resources, such as energy or bandwidth. 

Our stack features a rich application programming interface that is centered around the notion of a *timeline* -- a virtual sense of time to which each applications bind with a desired *accuracy interval* and *minimum resolution*. This enables developers to easily write choreographed applications that are capable of requesting and observing local time uncertainty. 

The stack works by constructing a timing subsystem in Linux that runs in parallel to timekeeping and POSIX timers. In this stack quality metrics are passed alongside time calls. The Linux implementation builds upon existing open source software, such as NTP, LinuxPTP and OpenSplice to provide a limited subset of features to illustrate the usefulness of our architecture.

# License #

Copyright (c) Regents of the University of California, 2015.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# Installation and Usage #

Please refer to the [wiki](https://bitbucket.org/rose-line/qot-stack/wiki).

# Contributors #

**University of California Los Angeles (UCLA)**

* Mani Srivastava
* Sudhakar Pamarti
* Andrew Symington
* Hani Esmaeelzadeh
* Amr Alanwar
* Fatima Muhammad
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

# Development #

This code is maintained by University of California Los Angeles (UCLA) on behalf of the RoseLine project. If you wish to contribute to development, please fork the repository, submit your changes to a new branch, and submit the updated code by means of a pull request against origin/master.

# Support #

Questions can be directed to Andrew Symington (asymingt@ucla.edu).