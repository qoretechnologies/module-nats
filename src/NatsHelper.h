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
