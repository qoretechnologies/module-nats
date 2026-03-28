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

        int64 effective_timeout = infinite ? QORE_IO_POLL_INTERVAL_MS :
            (remaining_ms > QORE_IO_POLL_INTERVAL_MS ? QORE_IO_POLL_INTERVAL_MS : remaining_ms);

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

        if (s == NATS_MAX_DELIVERED_MSGS || s == NATS_INVALID_SUBSCRIPTION) {
            // Auto-unsubscribe limit reached or subscription drained/closed - not an error
            return nullptr;
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

QoreListNode* QoreNatsSubscription::fetch(int batch, int64 timeout_ms, ExceptionSink* xsink) {
    if (!sub) {
        xsink->raiseException("NATS-SUBSCRIBE-ERROR", "subscription is not valid");
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

        int64 effective_timeout = (remaining_ms > QORE_IO_POLL_INTERVAL_MS)
            ? QORE_IO_POLL_INTERVAL_MS : remaining_ms;

        natsMsgList list{};
        jsErrCode jerr{};
        natsStatus s = natsSubscription_Fetch(&list, sub, batch, effective_timeout, &jerr);

        if (s == NATS_OK) {
            ReferenceHolder<QoreListNode> result(
                new QoreListNode(hashdeclNatsMsgInfo->getTypeInfo(true)), xsink);
            for (int i = 0; i < list.Count; ++i) {
                if (lastMsg) {
                    natsMsg_Destroy(lastMsg);
                }
                lastMsg = list.Msgs[i];
                list.Msgs[i] = nullptr;

                QoreHashNode* h = nats_msg_to_hash(lastMsg, xsink);
                if (*xsink) {
                    natsMsgList_Destroy(&list);
                    return nullptr;
                }
                result->push(h, xsink);
            }
            natsMsgList_Destroy(&list);
            return result.release();
        }

        if (s == NATS_TIMEOUT) {
            remaining_ms -= effective_timeout;
            if (remaining_ms <= 0) {
                return new QoreListNode(hashdeclNatsMsgInfo->getTypeInfo(true));
            }
            continue;
        }

        nats_error(xsink, "NATS-SUBSCRIBE-ERROR", s, "failed to fetch messages");
        return nullptr;
    }
}

int QoreNatsSubscription::waitForDrainCompletion(int64 timeout_ms, ExceptionSink* xsink) {
    if (!sub) {
        xsink->raiseException("NATS-SUBSCRIBE-ERROR", "subscription is not valid");
        return -1;
    }

    if (qore_check_cancel(xsink)) {
        return -1;
    }

    int64 remaining_ms = timeout_ms;

    while (true) {
        if (qore_check_cancel(xsink)) {
            return -1;
        }

        int64 effective_timeout = (remaining_ms > QORE_IO_POLL_INTERVAL_MS)
            ? QORE_IO_POLL_INTERVAL_MS : remaining_ms;

        natsStatus s = natsSubscription_WaitForDrainCompletion(sub, effective_timeout);
        if (s == NATS_OK) {
            return 0;
        }

        if (s == NATS_TIMEOUT) {
            remaining_ms -= effective_timeout;
            if (remaining_ms <= 0) {
                nats_error(xsink, "NATS-SUBSCRIBE-ERROR", s,
                    "failed waiting for drain completion");
                return -1;
            }
            continue;
        }

        nats_error(xsink, "NATS-SUBSCRIBE-ERROR", s,
            "failed waiting for drain completion");
        return -1;
    }
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

int QoreNatsSubscription::nakWithDelay(int64 delay_ms, ExceptionSink* xsink) {
    if (!lastMsg) {
        xsink->raiseException("NATS-SUBSCRIBE-ERROR",
            "no message to negative-acknowledge with delay");
        return -1;
    }
    natsStatus s = natsMsg_NakWithDelay(lastMsg, delay_ms * 1000000LL, nullptr);
    if (s != NATS_OK) {
        nats_error(xsink, "NATS-SUBSCRIBE-ERROR", s,
            "failed to negative-acknowledge message with delay");
        return -1;
    }
    return 0;
}

const char* QoreNatsSubscription::getSubject() const {
    if (!sub) {
        return "";
    }
    return natsSubscription_GetSubject(sub);
}

int64 QoreNatsSubscription::getId() const {
    if (!sub) {
        return -1;
    }
    return natsSubscription_GetID(sub);
}

QoreHashNode* QoreNatsSubscription::getPending(ExceptionSink* xsink) {
    if (!sub) {
        xsink->raiseException("NATS-SUBSCRIBE-ERROR", "subscription is not valid");
        return nullptr;
    }
    int msgs = 0, bytes = 0;
    natsStatus s = natsSubscription_GetPending(sub, &msgs, &bytes);
    if (s != NATS_OK) {
        nats_error(xsink, "NATS-SUBSCRIBE-ERROR", s, "failed to get pending counts");
        return nullptr;
    }
    ReferenceHolder<QoreHashNode> h(new QoreHashNode(autoTypeInfo), xsink);
    h->setKeyValue("msgs", (int64)msgs, xsink);
    if (!*xsink) {
        h->setKeyValue("bytes", (int64)bytes, xsink);
    }
    if (*xsink) {
        return nullptr;
    }
    return h.release();
}

int64 QoreNatsSubscription::getDelivered(ExceptionSink* xsink) {
    if (!sub) {
        xsink->raiseException("NATS-SUBSCRIBE-ERROR", "subscription is not valid");
        return -1;
    }
    int64_t msgs = 0;
    natsStatus s = natsSubscription_GetDelivered(sub, &msgs);
    if (s != NATS_OK) {
        nats_error(xsink, "NATS-SUBSCRIBE-ERROR", s, "failed to get delivered count");
        return -1;
    }
    return (int64)msgs;
}

int64 QoreNatsSubscription::getDropped(ExceptionSink* xsink) {
    if (!sub) {
        xsink->raiseException("NATS-SUBSCRIBE-ERROR", "subscription is not valid");
        return -1;
    }
    int64_t msgs = 0;
    natsStatus s = natsSubscription_GetDropped(sub, &msgs);
    if (s != NATS_OK) {
        nats_error(xsink, "NATS-SUBSCRIBE-ERROR", s, "failed to get dropped count");
        return -1;
    }
    return (int64)msgs;
}

QoreHashNode* QoreNatsSubscription::getSubscriptionStats(ExceptionSink* xsink) {
    if (!sub) {
        xsink->raiseException("NATS-SUBSCRIBE-ERROR", "subscription is not valid");
        return nullptr;
    }

    int pendingMsgs = 0, pendingBytes = 0;
    int maxPendingMsgs = 0, maxPendingBytes = 0;
    int64_t deliveredMsgs = 0, droppedMsgs = 0;
    natsStatus s = natsSubscription_GetStats(sub, &pendingMsgs, &pendingBytes,
        &maxPendingMsgs, &maxPendingBytes, &deliveredMsgs, &droppedMsgs);
    if (s != NATS_OK) {
        nats_error(xsink, "NATS-SUBSCRIBE-ERROR", s, "failed to get subscription stats");
        return nullptr;
    }

    ReferenceHolder<QoreHashNode> h(new QoreHashNode(hashdeclNatsSubscriptionStats, xsink), xsink);
    if (*xsink) {
        return nullptr;
    }

    const char* subject = natsSubscription_GetSubject(sub);
    if (subject) {
        h->setKeyValue("subject", new QoreStringNode(subject), xsink);
        if (*xsink) {
            return nullptr;
        }
    }
    if (!*xsink) {
        h->setKeyValue("id", (int64)natsSubscription_GetID(sub), xsink);
    }
    if (!*xsink) {
        h->setKeyValue("pending_msgs", (int64)pendingMsgs, xsink);
    }
    if (!*xsink) {
        h->setKeyValue("pending_bytes", (int64)pendingBytes, xsink);
    }
    if (!*xsink) {
        h->setKeyValue("delivered_msgs", (int64)deliveredMsgs, xsink);
    }
    if (!*xsink) {
        h->setKeyValue("dropped_msgs", (int64)droppedMsgs, xsink);
    }
    if (*xsink) {
        return nullptr;
    }
    return h.release();
}

int QoreNatsSubscription::setPendingLimits(int msg_limit, int bytes_limit,
        ExceptionSink* xsink) {
    if (!sub) {
        xsink->raiseException("NATS-SUBSCRIBE-ERROR", "subscription is not valid");
        return -1;
    }
    natsStatus s = natsSubscription_SetPendingLimits(sub, msg_limit, bytes_limit);
    if (s != NATS_OK) {
        nats_error(xsink, "NATS-SUBSCRIBE-ERROR", s, "failed to set pending limits");
        return -1;
    }
    return 0;
}

QoreHashNode* QoreNatsSubscription::getMetadata(ExceptionSink* xsink) {
    if (!lastMsg) {
        return nullptr;
    }

    jsMsgMetaData* meta = nullptr;
    natsStatus s = natsMsg_GetMetaData(&meta, lastMsg);
    if (s != NATS_OK) {
        // Not a JetStream message or no metadata - not an error
        return nullptr;
    }

    ReferenceHolder<QoreHashNode> h(new QoreHashNode(hashdeclNatsJSMsgMetadata, xsink), xsink);
    if (*xsink) {
        jsMsgMetaData_Destroy(meta);
        return nullptr;
    }

    if (meta->Domain) {
        h->setKeyValue("domain", new QoreStringNode(meta->Domain), xsink);
    }
    if (!*xsink && meta->Stream) {
        h->setKeyValue("stream", new QoreStringNode(meta->Stream), xsink);
    }
    if (!*xsink && meta->Consumer) {
        h->setKeyValue("consumer", new QoreStringNode(meta->Consumer), xsink);
    }
    if (!*xsink) {
        h->setKeyValue("num_delivered", (int64)meta->NumDelivered, xsink);
    }
    if (!*xsink) {
        h->setKeyValue("stream_seq", (int64)meta->Sequence.Stream, xsink);
    }
    if (!*xsink) {
        h->setKeyValue("consumer_seq", (int64)meta->Sequence.Consumer, xsink);
    }
    if (!*xsink) {
        h->setKeyValue("num_pending", (int64)meta->NumPending, xsink);
    }
    if (!*xsink && meta->Timestamp > 0) {
        int64 us = meta->Timestamp / 1000;
        h->setKeyValue("timestamp",
            DateTimeNode::makeAbsolute(currentTZ(), us / 1000000, (int)(us % 1000000)),
            xsink);
    }

    jsMsgMetaData_Destroy(meta);

    if (*xsink) {
        return nullptr;
    }
    return h.release();
}
