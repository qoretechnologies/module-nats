/* -*- mode: c++; indent-tabs-mode: nil -*- */
/** @file QoreNatsSubscription.h QoreNatsSubscription class definition */
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

#ifndef _QORE_NATS_SUBSCRIPTION_H
#define _QORE_NATS_SUBSCRIPTION_H

#include "nats-module.h"
#include "NatsHelper.h"

//! C++ wrapper for natsSubscription
class QoreNatsSubscription : public AbstractPrivateData {
public:
    //! Constructor - takes ownership of the subscription
    DLLLOCAL QoreNatsSubscription(natsSubscription* sub);

    //! Destructor
    DLLLOCAL virtual ~QoreNatsSubscription();

    //! Get next message with interruptible polling
    /** Uses 500ms polling intervals with qore_check_cancel() checks
        @param timeout_ms total timeout in milliseconds
        @param xsink exception sink
        @return hash<NatsMsgInfo> or nullptr on timeout/error
    */
    DLLLOCAL QoreHashNode* nextMsg(int64 timeout_ms, ExceptionSink* xsink);

    //! Unsubscribe
    DLLLOCAL int unsubscribe(ExceptionSink* xsink);

    //! Auto-unsubscribe after max messages
    DLLLOCAL int autoUnsubscribe(int max, ExceptionSink* xsink);

    //! Drain the subscription
    DLLLOCAL int drain(ExceptionSink* xsink);

    //! Wait for drain completion
    DLLLOCAL int waitForDrainCompletion(int64 timeout_ms, ExceptionSink* xsink);

    //! Check if valid
    DLLLOCAL bool isValid() const;

    //! Fetch messages from a pull subscription
    DLLLOCAL QoreListNode* fetch(int batch, int64 timeout_ms, ExceptionSink* xsink);

    //! Get pending message count
    DLLLOCAL int64 getPendingMsgs(ExceptionSink* xsink) const;

    //! Store the last received message for JetStream ack operations
    DLLLOCAL void setLastMsg(natsMsg* msg);

    //! JetStream: acknowledge the last received message
    DLLLOCAL int ack(ExceptionSink* xsink);

    //! JetStream: negative-acknowledge the last received message
    DLLLOCAL int nak(ExceptionSink* xsink);

    //! JetStream: signal that processing is in progress
    DLLLOCAL int inProgress(ExceptionSink* xsink);

    //! JetStream: terminate delivery of the last message
    DLLLOCAL int term(ExceptionSink* xsink);

    //! Negative acknowledge with redelivery delay
    DLLLOCAL int nakWithDelay(int64 delay_ms, ExceptionSink* xsink);

    //! Get JetStream metadata for the last received message
    DLLLOCAL QoreHashNode* getMetadata(ExceptionSink* xsink);

    //! Get subscription subject
    DLLLOCAL const char* getSubject() const;

    //! Get subscription ID
    DLLLOCAL int64 getId() const;

    //! Get pending message and byte counts
    DLLLOCAL QoreHashNode* getPending(ExceptionSink* xsink);

    //! Get delivered message count
    DLLLOCAL int64 getDelivered(ExceptionSink* xsink);

    //! Get dropped message count
    DLLLOCAL int64 getDropped(ExceptionSink* xsink);

    //! Get subscription stats
    DLLLOCAL QoreHashNode* getSubscriptionStats(ExceptionSink* xsink);

    //! Set pending message and byte limits
    DLLLOCAL int setPendingLimits(int msg_limit, int bytes_limit, ExceptionSink* xsink);

private:
    natsSubscription* sub = nullptr;
    natsMsg* lastMsg = nullptr;

    // non-copyable
    QoreNatsSubscription(const QoreNatsSubscription&) = delete;
    QoreNatsSubscription& operator=(const QoreNatsSubscription&) = delete;
};

#endif // _QORE_NATS_SUBSCRIPTION_H
