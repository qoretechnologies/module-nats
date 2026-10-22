/* -*- mode: c++; indent-tabs-mode: nil -*- */
/** @file NatsHelper.h NATS helper functions */
/*
    Qore nats module

    Copyright (C) 2026 Qore Technologies, s.r.o.

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/

#ifndef _QORE_NATS_HELPER_H
#define _QORE_NATS_HELPER_H

#include "nats-module.h"

#include <cstdarg>
#include <memory>
#include <mutex>

//! Raise a Qore exception from a natsStatus error code
/** @param xsink the exception sink
    @param err the Qore exception error code (e.g. "NATS-CONNECTION-ERROR")
    @param s the natsStatus error code
    @param fmt printf-style format string for the description
*/
DLLLOCAL void nats_error(ExceptionSink* xsink, const char* err, natsStatus s,
    const char* fmt, ...);

//! Raise a Qore exception from a natsStatus error code with JetStream error code
/** @param xsink the exception sink
    @param err the Qore exception error code
    @param s the natsStatus error code
    @param jerr the JetStream-specific error code
    @param fmt printf-style format string for the description
*/
DLLLOCAL void nats_js_error(ExceptionSink* xsink, const char* err, natsStatus s,
    jsErrCode jerr, const char* fmt, ...);

//! Raise a Qore exception from a microError
/** @param xsink the exception sink
    @param err the Qore exception error code (e.g. "NATS-MICRO-ERROR")
    @param me the microError (will be destroyed by this function)
    @param fmt printf-style format string for the description
*/
DLLLOCAL void nats_micro_error(ExceptionSink* xsink, const char* err, microError* me,
    const char* fmt, ...);

//! Convert a natsMsg to a hash<NatsMsgInfo>
/** @param msg the NATS message (caller retains ownership)
    @param xsink the exception sink
    @return a hash<NatsMsgInfo> or nullptr on error
*/
DLLLOCAL QoreHashNode* nats_msg_to_hash(natsMsg* msg, ExceptionSink* xsink);

//! Check sandbox network access for a NATS URL
/** @param url the NATS server URL (nats://host:port or tls://host:port)
    @param xsink the exception sink
    @return 0 on success, -1 if access was denied
*/
DLLLOCAL int check_nats_network_access(const char* url, ExceptionSink* xsink);

//! Makes the TCP connections of a NATS connection under the network sandbox of the code that created it
/** The NATS C client makes the first connection of a NATS connection on the thread that creates it, and every
    reconnection on its own reconnect thread, which %Qore did not start: to the configured servers, and to the servers
    that the cluster reports (discovered servers), whose addresses come from the server.  When a sandbox governs the
    code that creates the connection, the dialer is installed as the client's proxy connection handler, so that every
    connection is made under that sandbox, on whatever thread: the host name is resolved, and each address is checked
    against the network policy before it is connected to, exactly like the first connection.

    The dialer must outlive the C client connection, whose reconnect thread uses it; natsConnection_Destroy() joins
    that thread.
*/
class NatsSandboxDialer {
public:
    //! Captures the sandbox of the current thread
    /** @param timeout_ms the connect timeout of the connection (\c connect_timeout_ms); 0 for the C client's default
        @param order_ip the IP resolution order of the connection (\c ip_resolution_order): 4, 6, 46, 64, or 0 for
        any
        @param no_randomize true if the addresses of a host are tried in the order that the resolver returns them
    */
    DLLLOCAL NatsSandboxDialer(int64 timeout_ms, int order_ip, bool no_randomize);

    //! Returns true if a sandbox governed the code that created the dialer
    DLLLOCAL bool isSandboxed() const {
        return sandbox.isSandboxed();
    }

    //! The C client's proxy connection handler; \a closure is the dialer
    DLLLOCAL static natsStatus connect(natsSock* fd, char* host, int port, void* closure);

    //! Moves an exception raised while the first connection was made (a cancellation) to \a xsink
    /** The first connection is made on the %Qore thread that creates the connection, which can receive the exception;
        called when natsConnection_Connect() returns, after which an exception raised on another thread is discarded
    */
    DLLLOCAL void takeException(ExceptionSink* xsink);

private:
    //! the sandbox of the code that created the connection
    QoreSandboxCapture sandbox;
    //! the connect timeout in milliseconds
    int64 timeout_ms;
    //! the IP resolution order: 4, 6, 46, 64, or 0 for any
    int order_ip;
    //! true if the addresses of a host are not shuffled
    bool no_randomize;

    //! protects pending and initial
    std::mutex m;
    //! an exception raised while the first connection was made
    std::unique_ptr<ExceptionSink> pending;
    //! true until takeException() is called
    bool initial = true;

    DLLLOCAL natsStatus connectIntern(natsSock* fd, const char* host, int port);
    DLLLOCAL natsStatus connectAddress(natsSock* fd, const struct addrinfo* ai, int64 timeout_ms,
        ExceptionSink& xsink);
    DLLLOCAL void keepException(ExceptionSink& xsink);

    // non-copyable
    NatsSandboxDialer(const NatsSandboxDialer&) = delete;
    NatsSandboxDialer& operator=(const NatsSandboxDialer&) = delete;
};

//! Check sandbox filesystem access for a file path
/** @param path the file path to check
    @param xsink the exception sink
    @return 0 on success (or no sandbox active), -1 if access was denied
*/
DLLLOCAL int check_nats_file_access(const char* path, ExceptionSink* xsink);

//! Convert a kvEntry to a hash<NatsKVEntry>
/** @param entry the KV entry (caller retains ownership)
    @param xsink the exception sink
    @return a hash<NatsKVEntry> or nullptr on error
*/
DLLLOCAL QoreHashNode* nats_kv_entry_to_hash(kvEntry* entry, ExceptionSink* xsink);

//! RAII helper for natsMsg destruction
class NatsMsgHolder {
public:
    DLLLOCAL NatsMsgHolder(natsMsg* msg = nullptr) : msg(msg) {}
    DLLLOCAL ~NatsMsgHolder() {
        if (msg) {
            natsMsg_Destroy(msg);
        }
    }
    DLLLOCAL natsMsg* operator*() const { return msg; }
    DLLLOCAL natsMsg** operator&() { return &msg; }
    DLLLOCAL natsMsg* release() {
        natsMsg* rv = msg;
        msg = nullptr;
        return rv;
    }
    DLLLOCAL operator bool() const { return msg != nullptr; }

private:
    natsMsg* msg;

    // non-copyable
    NatsMsgHolder(const NatsMsgHolder&) = delete;
    NatsMsgHolder& operator=(const NatsMsgHolder&) = delete;
};

#endif // _QORE_NATS_HELPER_H
