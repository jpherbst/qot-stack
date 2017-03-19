# @file CMakeLists.txt
# @brief Base CMake build script for the qot-stack project
# @author Andrew Symington
#
# Copyright (c) Regents of the University of California, 2015.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 	1. Redistributions of source code must retain the above copyright notice,
#     this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright notice,
#     this list of conditions and the following disclaimer in the documentation
#     and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

# Hardware-agnostic part of the qot stack
ADD_SUBDIRECTORY(qot)

# Platform clocks (should not rely on any architecture-specific code)
ADD_SUBDIRECTORY(qot_chronos)

# Network clocks (should not rely on any architecture-specific code)
ADD_SUBDIRECTORY(cc1200)
ADD_SUBDIRECTORY(cc2520)
ADD_SUBDIRECTORY(dw1000)

# Hardware-specific am335x components
IF (CROSS_AM335X)
	ADD_SUBDIRECTORY(qot_am335x)
ENDIF (CROSS_AM335X)

# Generic (or x86) Driver for Platforms that lack pins
IF (X86_64)
	ADD_SUBDIRECTORY(qot_x86)
ENDIF (X86_64)
