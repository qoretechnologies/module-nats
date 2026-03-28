/* -*- mode: c++; indent-tabs-mode: nil -*- */
/** @file QoreNatsConnection.h QoreNatsConnection class definition */
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

#ifndef _QORE_NATS_CONNECTION_H
#define _QORE_NATS_CONNECTION_H

#include "nats-module.h"
#include "NatsHelper.h"

//! C++ wrapper for natsConnection + natsOptions
class QoreNatsConnection : public AbstractPrivateData {
public:
    //! Constructor with URL string
    DLLLOCAL QoreNatsConnection(const char* url, ExceptionSink* xsink);

    //! Constructor with options hash
    DLLLOCAL QoreNatsConnection(const QoreHashNode* opts, ExceptionSink* xsink);

    //! Destructor
    DLLLOCAL virtual ~QoreNatsConnection();

    //! Publish a message
    DLLLOCAL int publish(const char* subject, const void* data, int data_len,
        ExceptionSink* xsink);

    //! Publish a message with headers
    DLLLOCAL int publishMsg(const char* subject, const void* data, int data_len,
        const QoreHashNode* headers, ExceptionSink* xsink);

    //! Request/reply
    DLLLOCAL QoreHashNode* request(const char* subject, const void* data, int data_len,
        int64 timeout_ms, ExceptionSink* xsink);

    //! Subscribe to a subject
    DLLLOCAL QoreNatsSubscription* subscribe(const char* subject, ExceptionSink* xsink);

    //! Queue subscribe to a subject
    DLLLOCAL QoreNatsSubscription* queueSubscribe(const char* subject, const char* queue,
        ExceptionSink* xsink);

    //! Create a JetStream context
    DLLLOCAL QoreNatsJetStream* jetStream(ExceptionSink* xsink);

    //! Drain the connection
    DLLLOCAL int drain(ExceptionSink* xsink);

    //! Close the connection
    DLLLOCAL void close();

    //! Flush pending data
    DLLLOCAL int flush(int64 timeout_ms, ExceptionSink* xsink);

    //! Check if connected
    DLLLOCAL bool isConnected() const;

    //! Get connection status
    DLLLOCAL int status() const;

    //! Get the URL of the currently connected server
    DLLLOCAL QoreStringNode* getConnectedUrl(ExceptionSink* xsink);

    //! Get the server ID
    DLLLOCAL QoreStringNode* getServerId(ExceptionSink* xsink);

    //! Get the client ID assigned by the server
    DLLLOCAL int64 getClientId(ExceptionSink* xsink);

    //! Get the client IP as seen by the server
    DLLLOCAL QoreStringNode* getClientIp(ExceptionSink* xsink);

    //! Get round-trip time to the server in milliseconds
    DLLLOCAL QoreValue getRtt(ExceptionSink* xsink);

    //! Get maximum payload size allowed by the server
    DLLLOCAL int64 getMaxPayload(ExceptionSink* xsink);

    //! Check if the server supports message headers
    DLLLOCAL bool hasHeaderSupport(ExceptionSink* xsink);

    //! Get the local IP and port used for the connection
    DLLLOCAL QoreHashNode* getLocalIpAndPort(ExceptionSink* xsink);

    //! Check if the connection is reconnecting
    DLLLOCAL bool isReconnecting() const;

    //! Check if the connection is draining
    DLLLOCAL bool isDraining() const;

    //! Get the number of bytes buffered for write
    DLLLOCAL int buffered() const;

    //! Get connection statistics
    DLLLOCAL QoreHashNode* getStats(ExceptionSink* xsink);

    //! Get list of known server URLs
    DLLLOCAL QoreListNode* getServers(ExceptionSink* xsink);

    //! Get list of discovered server URLs
    DLLLOCAL QoreListNode* getDiscoveredServers(ExceptionSink* xsink);

    //! Force reconnect to another server
    DLLLOCAL int reconnect(ExceptionSink* xsink);

    //! Get comprehensive connection info
    DLLLOCAL QoreHashNode* getConnectionInfo(ExceptionSink* xsink);

    //! Get the raw natsConnection pointer
    DLLLOCAL natsConnection* getConnection() const { return conn; }

private:
    natsConnection* conn = nullptr;
    natsOptions* opts = nullptr;

    //! Configure options from a hash
    DLLLOCAL int configureOptions(const QoreHashNode* options, ExceptionSink* xsink);

    // non-copyable
    QoreNatsConnection(const QoreNatsConnection&) = delete;
    QoreNatsConnection& operator=(const QoreNatsConnection&) = delete;
};

#endif // _QORE_NATS_CONNECTION_H
