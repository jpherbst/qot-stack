/*
 * @file messenger.h
 * @brief A simple C application programming interface to the QoT stack DDS Messenger Service in C++
 * @author Sandeep D'souza
 *
 * Copyright (c) Carnegie Mellon University, 2016.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

// #ifdef __cplusplus
// #  define EXTERNC extern "C"
// #  define NOTHROW noexcept
// #else
// #  define EXTERNC
// #  define NOTHROW
// #endif

#include <vector>
#include <string>

extern "C"
{
	#include "../../../qot_types.h"
}

/* Alias for your object in C that hides the implementation */
typedef void* messenger_t;

/* Creates the messenger object using the first constructor */
messenger_t create_messenger(const char *name, const char *uuid);

/* Frees the messenger object, using delete */
void delete_messenger(messenger_t messenger);

/* Publish Message*/
qot_return_t publish_message(messenger_t messenger, qot_message_t msg);

/* Define the Nodes participating in the coordination */
qot_return_t define_cluster(messenger_t messenger, const std::vector<std::string> Nodes);

/* Wait for all peers to join the cluster*/
qot_return_t wait_for_peers_to_join(messenger_t messenger);
