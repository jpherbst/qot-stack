# Content description #

This folder contains the QoT stack source code, divided into the following five distinct modules.

* **api** - the Application Programming Interface that developers use to create, bind, destroy and interact with timelines.
* **examples** - example projects showing how to use the QoT stack.
* **modules** - core and clock kernel modules.
* **service** - the system daemon responsible for synchronization.
* **test** - unit tests for the system components.
* **utils** - user-space tools for introspection and debugging.
* **virt** - code to run the QoT Stack on a virtualized platform.

All five modules above rely on the file **qot_types.h**, which defines the fundamental time types in the system, basic uncertain time mathematics, and kernel-userspace ioctl message types.
