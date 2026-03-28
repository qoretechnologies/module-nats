/* -*- mode: c++; indent-tabs-mode: nil -*- */
/** @file QoreNatsKVWatcher.cpp QoreNatsKVWatcher implementation */
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

#include "QoreNatsKVWatcher.h"

QoreNatsKVWatcher::QoreNatsKVWatcher(kvWatcher* w) : watcher(w) {
}

QoreNatsKVWatcher::~QoreNatsKVWatcher() {
    if (watcher) {
        kvWatcher_Destroy(watcher);
        watcher = nullptr;
    }
}

QoreHashNode* QoreNatsKVWatcher::next(int64 timeout_ms, ExceptionSink* xsink) {
    if (!watcher) {
        xsink->raiseException("NATS-KV-WATCH-ERROR", "watcher is not valid");
        return nullptr;
    }

    if (qore_check_cancel(xsink)) {
        return nullptr;
    }

    int64 remaining_ms = timeout_ms;

    while (true) {
        if (qore_check_cancel(xsink)) {
            return nullptr;
        }

        int64 effective = (remaining_ms > QORE_IO_POLL_INTERVAL_MS)
            ? QORE_IO_POLL_INTERVAL_MS : remaining_ms;

        kvEntry* entry = nullptr;
        natsStatus s = kvWatcher_Next(&entry, watcher, effective);
        if (s == NATS_OK && entry) {
            QoreHashNode* rv = nats_kv_entry_to_hash(entry, xsink);
            kvEntry_Destroy(entry);
            return rv;
        }

        if (s == NATS_TIMEOUT) {
            remaining_ms -= effective;
            if (remaining_ms <= 0) {
                return nullptr;  // Timeout, not an error
            }
            continue;
        }

        if (s != NATS_OK) {
            nats_error(xsink, "NATS-KV-WATCH-ERROR", s, "watcher next failed");
            return nullptr;
        }
    }
}

int QoreNatsKVWatcher::stop(ExceptionSink* xsink) {
    if (!watcher) {
        xsink->raiseException("NATS-KV-WATCH-ERROR", "watcher is not valid");
        return -1;
    }
    natsStatus s = kvWatcher_Stop(watcher);
    if (s != NATS_OK) {
        nats_error(xsink, "NATS-KV-WATCH-ERROR", s, "failed to stop watcher");
        return -1;
    }
    return 0;
}
