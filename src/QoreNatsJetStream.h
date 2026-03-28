/* -*- mode: c++; indent-tabs-mode: nil -*- */
/** @file QoreNatsJetStream.h QoreNatsJetStream class definition */
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

#ifndef _QORE_NATS_JETSTREAM_H
#define _QORE_NATS_JETSTREAM_H

#include "nats-module.h"
#include "NatsHelper.h"

class QoreNatsSubscription;
class QoreNatsKVStore;

//! C++ wrapper for jsCtx (JetStream context)
class QoreNatsJetStream : public AbstractPrivateData {
public:
    //! Constructor - creates a JetStream context from a connection
    DLLLOCAL QoreNatsJetStream(natsConnection* conn, ExceptionSink* xsink);

    //! Destructor
    DLLLOCAL virtual ~QoreNatsJetStream();

    // Stream CRUD
    DLLLOCAL QoreHashNode* addStream(const QoreHashNode* config, ExceptionSink* xsink);
    DLLLOCAL QoreHashNode* updateStream(const QoreHashNode* config, ExceptionSink* xsink);
    DLLLOCAL int deleteStream(const char* name, ExceptionSink* xsink);
    DLLLOCAL QoreHashNode* getStreamInfo(const char* name, ExceptionSink* xsink);
    DLLLOCAL int purgeStream(const char* name, ExceptionSink* xsink);

    // Consumer CRUD
    DLLLOCAL QoreHashNode* addConsumer(const char* stream, const QoreHashNode* config,
        ExceptionSink* xsink);
    DLLLOCAL int deleteConsumer(const char* stream, const char* consumer,
        ExceptionSink* xsink);
    DLLLOCAL QoreHashNode* getConsumerInfo(const char* stream, const char* consumer,
        ExceptionSink* xsink);

    // Publish with ack
    DLLLOCAL QoreHashNode* publish(const char* subject, const void* data, int data_len,
        ExceptionSink* xsink);

    //! Publish with options
    DLLLOCAL QoreHashNode* publishWithOptions(const char* subject, const void* data,
        int data_len, const QoreHashNode* pub_opts, ExceptionSink* xsink);

    //! Get JetStream account info
    DLLLOCAL QoreHashNode* getAccountInfo(ExceptionSink* xsink);

    // Subscribe (push)
    DLLLOCAL QoreNatsSubscription* subscribe(const char* subject, ExceptionSink* xsink);

    // Pull subscribe
    DLLLOCAL QoreNatsSubscription* pullSubscribe(const char* subject, const char* durable,
        ExceptionSink* xsink);

    // KV operations
    DLLLOCAL QoreNatsKVStore* keyValue(const char* bucket, ExceptionSink* xsink);
    DLLLOCAL QoreNatsKVStore* createKeyValue(const QoreHashNode* config, ExceptionSink* xsink);
    DLLLOCAL int deleteKeyValue(const char* bucket, ExceptionSink* xsink);

private:
    jsCtx* js = nullptr;
    natsConnection* conn = nullptr;  // borrowed reference

    //! Helper: configure jsStreamConfig from hash
    DLLLOCAL int configureStreamConfig(jsStreamConfig* cfg, const QoreHashNode* config,
        ExceptionSink* xsink);

    //! Helper: convert jsStreamInfo to hash
    DLLLOCAL QoreHashNode* streamInfoToHash(jsStreamInfo* info, ExceptionSink* xsink);

    //! Helper: configure jsConsumerConfig from hash
    DLLLOCAL int configureConsumerConfig(jsConsumerConfig* cfg, const QoreHashNode* config,
        ExceptionSink* xsink);

    //! Helper: convert jsConsumerInfo to hash
    DLLLOCAL QoreHashNode* consumerInfoToHash(jsConsumerInfo* info, ExceptionSink* xsink);

    // non-copyable
    QoreNatsJetStream(const QoreNatsJetStream&) = delete;
    QoreNatsJetStream& operator=(const QoreNatsJetStream&) = delete;
};

#endif // _QORE_NATS_JETSTREAM_H
