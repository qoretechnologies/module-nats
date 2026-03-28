/* -*- mode: c++; indent-tabs-mode: nil -*- */
/** @file QoreNatsKVWatcher.h QoreNatsKVWatcher class definition */
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

#ifndef _QORE_NATS_KV_WATCHER_H
#define _QORE_NATS_KV_WATCHER_H

#include "nats-module.h"
#include "NatsHelper.h"

//! C++ wrapper for kvWatcher
class QoreNatsKVWatcher : public AbstractPrivateData {
public:
    //! Constructor
    DLLLOCAL QoreNatsKVWatcher(kvWatcher* w);

    //! Destructor
    DLLLOCAL virtual ~QoreNatsKVWatcher();

    //! Get next KV entry update (interruptible polling)
    /** @param timeout_ms total timeout in milliseconds
        @param xsink exception sink
        @return hash<NatsKVEntry> or nullptr on timeout/error
    */
    DLLLOCAL QoreHashNode* next(int64 timeout_ms, ExceptionSink* xsink);

    //! Stop the watcher
    DLLLOCAL int stop(ExceptionSink* xsink);

private:
    kvWatcher* watcher = nullptr;

    QoreNatsKVWatcher(const QoreNatsKVWatcher&) = delete;
    QoreNatsKVWatcher& operator=(const QoreNatsKVWatcher&) = delete;
};

#endif // _QORE_NATS_KV_WATCHER_H
