/* -*- mode: c++; indent-tabs-mode: nil -*- */
/** @file QoreNatsSubscription.cpp QoreNatsSubscription implementation */
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

#include "QoreNatsSubscription.h"

QoreNatsSubscription::QoreNatsSubscription(natsSubscription* sub) : sub(sub) {
}

QoreNatsSubscription::~QoreNatsSubscription() {
    if (lastMsg) {
        natsMsg_Destroy(lastMsg);
        lastMsg = nullptr;
    }
    if (sub) {
        natsSubscription_Destroy(sub);
        sub = nullptr;
    }
}

QoreHashNode* QoreNatsSubscription::nextMsg(int64 timeout_ms, ExceptionSink* xsink) {
    if (!sub) {
        xsink->raiseException("NATS-SUBSCRIBE-ERROR", "subscription is not valid");
        return nullptr;
    }

    // Check for cancellation before starting
    if (qore_check_cancel(xsink)) {
        return nullptr;
    }

    int64 remaining_ms = timeout_ms;
    bool infinite = (timeout_ms < 0);

    while (true) {
        // Check for cancellation
        if (qore_check_cancel(xsink)) {
            return nullptr;
        }

        int64 effective_timeout = infinite ? NATS_IO_POLL_INTERVAL_MS :
            (remaining_ms > NATS_IO_POLL_INTERVAL_MS ? NATS_IO_POLL_INTERVAL_MS : remaining_ms);

        natsMsg* msg = nullptr;
        natsStatus s = natsSubscription_NextMsg(&msg, sub, effective_timeout);

        if (s == NATS_OK) {
            // Clear old lastMsg and store new one for JetStream ack
            if (lastMsg) {
                natsMsg_Destroy(lastMsg);
            }
            lastMsg = msg;

            // Convert to hash (does not destroy msg)
            return nats_msg_to_hash(msg, xsink);
        }

        if (s == NATS_TIMEOUT) {
            // Check if overall timeout exceeded
            if (!infinite) {
                remaining_ms -= effective_timeout;
                if (remaining_ms <= 0) {
                    // Timeout - return nullptr (not an error)
                    return nullptr;
                }
            }
            // Otherwise continue polling
            continue;
        }

        // Real error
        nats_error(xsink, "NATS-SUBSCRIBE-ERROR", s, "failed to get next message");
        return nullptr;
    }
}

int QoreNatsSubscription::unsubscribe(ExceptionSink* xsink) {
    if (!sub) {
        xsink->raiseException("NATS-SUBSCRIBE-ERROR", "subscription is not valid");
        return -1;
    }
    natsStatus s = natsSubscription_Unsubscribe(sub);
    if (s != NATS_OK) {
        nats_error(xsink, "NATS-SUBSCRIBE-ERROR", s, "failed to unsubscribe");
        return -1;
    }
    return 0;
}

int QoreNatsSubscription::autoUnsubscribe(int max, ExceptionSink* xsink) {
    if (!sub) {
        xsink->raiseException("NATS-SUBSCRIBE-ERROR", "subscription is not valid");
        return -1;
    }
    natsStatus s = natsSubscription_AutoUnsubscribe(sub, max);
    if (s != NATS_OK) {
        nats_error(xsink, "NATS-SUBSCRIBE-ERROR", s, "failed to auto-unsubscribe");
        return -1;
    }
    return 0;
}

int QoreNatsSubscription::drain(ExceptionSink* xsink) {
    if (!sub) {
        xsink->raiseException("NATS-SUBSCRIBE-ERROR", "subscription is not valid");
        return -1;
    }
    natsStatus s = natsSubscription_Drain(sub);
    if (s != NATS_OK) {
        nats_error(xsink, "NATS-SUBSCRIBE-ERROR", s, "failed to drain subscription");
        return -1;
    }
    return 0;
}

bool QoreNatsSubscription::isValid() const {
    return sub && natsSubscription_IsValid(sub);
}

int64 QoreNatsSubscription::getPendingMsgs(ExceptionSink* xsink) const {
    if (!sub) {
        xsink->raiseException("NATS-SUBSCRIBE-ERROR", "subscription is not valid");
        return -1;
    }
    int msgs = 0;
    int bytes = 0;
    natsStatus s = natsSubscription_GetPending(sub, &msgs, &bytes);
    if (s != NATS_OK) {
        nats_error(xsink, "NATS-SUBSCRIBE-ERROR", s, "failed to get pending count");
        return -1;
    }
    return msgs;
}

void QoreNatsSubscription::setLastMsg(natsMsg* msg) {
    if (lastMsg) {
        natsMsg_Destroy(lastMsg);
    }
    lastMsg = msg;
}

int QoreNatsSubscription::ack(ExceptionSink* xsink) {
    if (!lastMsg) {
        xsink->raiseException("NATS-JETSTREAM-ERROR", "no message to acknowledge");
        return -1;
    }
    natsStatus s = natsMsg_Ack(lastMsg, nullptr);
    if (s != NATS_OK) {
        nats_error(xsink, "NATS-JETSTREAM-ERROR", s, "failed to acknowledge message");
        return -1;
    }
    return 0;
}

int QoreNatsSubscription::nak(ExceptionSink* xsink) {
    if (!lastMsg) {
        xsink->raiseException("NATS-JETSTREAM-ERROR", "no message to negative-acknowledge");
        return -1;
    }
    natsStatus s = natsMsg_Nak(lastMsg, nullptr);
    if (s != NATS_OK) {
        nats_error(xsink, "NATS-JETSTREAM-ERROR", s,
            "failed to negative-acknowledge message");
        return -1;
    }
    return 0;
}

int QoreNatsSubscription::inProgress(ExceptionSink* xsink) {
    if (!lastMsg) {
        xsink->raiseException("NATS-JETSTREAM-ERROR",
            "no message to mark as in-progress");
        return -1;
    }
    natsStatus s = natsMsg_InProgress(lastMsg, nullptr);
    if (s != NATS_OK) {
        nats_error(xsink, "NATS-JETSTREAM-ERROR", s,
            "failed to mark message as in-progress");
        return -1;
    }
    return 0;
}

int QoreNatsSubscription::term(ExceptionSink* xsink) {
    if (!lastMsg) {
        xsink->raiseException("NATS-JETSTREAM-ERROR",
            "no message to terminate");
        return -1;
    }
    natsStatus s = natsMsg_Term(lastMsg, nullptr);
    if (s != NATS_OK) {
        nats_error(xsink, "NATS-JETSTREAM-ERROR", s,
            "failed to terminate message delivery");
        return -1;
    }
    return 0;
}
