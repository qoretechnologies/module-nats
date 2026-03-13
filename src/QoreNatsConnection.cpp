/* -*- mode: c++; indent-tabs-mode: nil -*- */
/** @file QoreNatsConnection.cpp QoreNatsConnection implementation */
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

#include "QoreNatsConnection.h"
#include "QoreNatsSubscription.h"
#include "QoreNatsJetStream.h"

QoreNatsConnection::QoreNatsConnection(const char* url, ExceptionSink* xsink) {
    // Check sandbox network access
    if (check_nats_network_access(url, xsink)) {
        return;
    }

    natsStatus s = natsOptions_Create(&opts);
    if (s != NATS_OK) {
        nats_error(xsink, "NATS-CONNECTION-ERROR", s, "failed to create NATS options");
        return;
    }

    s = natsOptions_SetURL(opts, url);
    if (s != NATS_OK) {
        nats_error(xsink, "NATS-CONNECTION-ERROR", s, "failed to set NATS URL '%s'", url);
        return;
    }

    s = natsConnection_Connect(&conn, opts);
    if (s != NATS_OK) {
        nats_error(xsink, "NATS-CONNECTION-ERROR", s, "failed to connect to '%s'", url);
    }
}

QoreNatsConnection::QoreNatsConnection(const QoreHashNode* options, ExceptionSink* xsink) {
    natsStatus s = natsOptions_Create(&opts);
    if (s != NATS_OK) {
        nats_error(xsink, "NATS-CONNECTION-ERROR", s, "failed to create NATS options");
        return;
    }

    if (configureOptions(options, xsink)) {
        return;
    }

    // Get URL for sandbox check
    QoreValue url_val = options->getKeyValue("url");
    if (url_val.getType() == NT_STRING) {
        if (check_nats_network_access(url_val.get<const QoreStringNode>()->c_str(), xsink)) {
            return;
        }
    }

    s = natsConnection_Connect(&conn, opts);
    if (s != NATS_OK) {
        nats_error(xsink, "NATS-CONNECTION-ERROR", s, "failed to connect");
    }
}

QoreNatsConnection::~QoreNatsConnection() {
    if (conn) {
        natsConnection_Destroy(conn);
        conn = nullptr;
    }
    if (opts) {
        natsOptions_Destroy(opts);
        opts = nullptr;
    }
}

int QoreNatsConnection::configureOptions(const QoreHashNode* options, ExceptionSink* xsink) {
    natsStatus s;

    // URL
    QoreValue v = options->getKeyValue("url");
    if (v.getType() == NT_STRING) {
        s = natsOptions_SetURL(opts, v.get<const QoreStringNode>()->c_str());
        if (s != NATS_OK) {
            nats_error(xsink, "NATS-CONNECTION-ERROR", s, "failed to set URL");
            return -1;
        }
    }

    // Token auth
    v = options->getKeyValue("token");
    if (v.getType() == NT_STRING) {
        s = natsOptions_SetToken(opts, v.get<const QoreStringNode>()->c_str());
        if (s != NATS_OK) {
            nats_error(xsink, "NATS-AUTH-ERROR", s, "failed to set token");
            return -1;
        }
    }

    // Username/password auth
    v = options->getKeyValue("username");
    QoreValue pw = options->getKeyValue("password");
    if (v.getType() == NT_STRING) {
        s = natsOptions_SetUserInfo(opts,
            v.get<const QoreStringNode>()->c_str(),
            pw.getType() == NT_STRING ? pw.get<const QoreStringNode>()->c_str() : nullptr);
        if (s != NATS_OK) {
            nats_error(xsink, "NATS-AUTH-ERROR", s, "failed to set user info");
            return -1;
        }
    }

    // NKey seed file
    v = options->getKeyValue("nkey_seed");
    if (v.getType() == NT_STRING) {
        QoreValue nkey_pub = options->getKeyValue("nkey_pub");
        s = natsOptions_SetNKeyFromSeed(opts,
            nkey_pub.getType() == NT_STRING
                ? nkey_pub.get<const QoreStringNode>()->c_str()
                : nullptr,
            v.get<const QoreStringNode>()->c_str());
        if (s != NATS_OK) {
            nats_error(xsink, "NATS-AUTH-ERROR", s, "failed to set NKey seed");
            return -1;
        }
    }

    // Credentials file
    v = options->getKeyValue("credentials");
    if (v.getType() == NT_STRING) {
        s = natsOptions_SetUserCredentialsFromFiles(opts,
            v.get<const QoreStringNode>()->c_str(), nullptr);
        if (s != NATS_OK) {
            nats_error(xsink, "NATS-AUTH-ERROR", s, "failed to set credentials file");
            return -1;
        }
    }

    // Connection name
    v = options->getKeyValue("name");
    if (v.getType() == NT_STRING) {
        s = natsOptions_SetName(opts, v.get<const QoreStringNode>()->c_str());
        if (s != NATS_OK) {
            nats_error(xsink, "NATS-CONNECTION-ERROR", s, "failed to set connection name");
            return -1;
        }
    }

    // Connect timeout
    v = options->getKeyValue("connect_timeout_ms");
    if (v.getType() == NT_INT) {
        s = natsOptions_SetTimeout(opts, v.getAsBigInt());
        if (s != NATS_OK) {
            nats_error(xsink, "NATS-CONNECTION-ERROR", s, "failed to set connect timeout");
            return -1;
        }
    }

    // Reconnect wait
    v = options->getKeyValue("reconnect_wait_ms");
    if (v.getType() == NT_INT) {
        s = natsOptions_SetReconnectWait(opts, v.getAsBigInt());
        if (s != NATS_OK) {
            nats_error(xsink, "NATS-CONNECTION-ERROR", s, "failed to set reconnect wait");
            return -1;
        }
    }

    // Max reconnect attempts
    v = options->getKeyValue("max_reconnect");
    if (v.getType() == NT_INT) {
        s = natsOptions_SetMaxReconnect(opts, (int)v.getAsBigInt());
        if (s != NATS_OK) {
            nats_error(xsink, "NATS-CONNECTION-ERROR", s, "failed to set max reconnect");
            return -1;
        }
    }

    // Ping interval
    v = options->getKeyValue("ping_interval");
    if (v.getType() == NT_INT) {
        s = natsOptions_SetPingInterval(opts, v.getAsBigInt());
        if (s != NATS_OK) {
            nats_error(xsink, "NATS-CONNECTION-ERROR", s, "failed to set ping interval");
            return -1;
        }
    }

    // Max pings out
    v = options->getKeyValue("max_pings_out");
    if (v.getType() == NT_INT) {
        s = natsOptions_SetMaxPingsOut(opts, (int)v.getAsBigInt());
        if (s != NATS_OK) {
            nats_error(xsink, "NATS-CONNECTION-ERROR", s, "failed to set max pings out");
            return -1;
        }
    }

    // No randomize
    v = options->getKeyValue("no_randomize");
    if (v.getType() == NT_BOOLEAN && v.getAsBool()) {
        s = natsOptions_SetNoRandomize(opts, true);
        if (s != NATS_OK) {
            nats_error(xsink, "NATS-CONNECTION-ERROR", s, "failed to set no randomize");
            return -1;
        }
    }

    // TLS options
    v = options->getKeyValue("tls");
    if (v.getType() == NT_HASH) {
        const QoreHashNode* tls = v.get<const QoreHashNode>();

        s = natsOptions_SetSecure(opts, true);
        if (s != NATS_OK) {
            nats_error(xsink, "NATS-TLS-ERROR", s, "failed to enable TLS");
            return -1;
        }

        QoreValue ca = tls->getKeyValue("ca_cert");
        if (ca.getType() == NT_STRING) {
            s = natsOptions_LoadCATrustedCertificates(opts,
                ca.get<const QoreStringNode>()->c_str());
            if (s != NATS_OK) {
                nats_error(xsink, "NATS-TLS-ERROR", s, "failed to load CA certificates");
                return -1;
            }
        }

        QoreValue cert = tls->getKeyValue("client_cert");
        QoreValue key = tls->getKeyValue("client_key");
        if (cert.getType() == NT_STRING && key.getType() == NT_STRING) {
            s = natsOptions_LoadCertificatesChain(opts,
                cert.get<const QoreStringNode>()->c_str(),
                key.get<const QoreStringNode>()->c_str());
            if (s != NATS_OK) {
                nats_error(xsink, "NATS-TLS-ERROR", s,
                    "failed to load client certificate/key");
                return -1;
            }
        }

        QoreValue skip = tls->getKeyValue("skip_verify");
        if (skip.getType() == NT_BOOLEAN && skip.getAsBool()) {
            s = natsOptions_SkipServerVerification(opts, true);
            if (s != NATS_OK) {
                nats_error(xsink, "NATS-TLS-ERROR", s,
                    "failed to set skip server verification");
                return -1;
            }
        }
    }

    return 0;
}

int QoreNatsConnection::publish(const char* subject, const void* data, int data_len,
        ExceptionSink* xsink) {
    if (!conn) {
        xsink->raiseException("NATS-PUBLISH-ERROR", "not connected");
        return -1;
    }
    natsStatus s = natsConnection_Publish(conn, subject, data, data_len);
    if (s != NATS_OK) {
        nats_error(xsink, "NATS-PUBLISH-ERROR", s,
            "failed to publish to subject '%s'", subject);
        return -1;
    }
    return 0;
}

int QoreNatsConnection::publishMsg(const char* subject, const void* data, int data_len,
        const QoreHashNode* headers, ExceptionSink* xsink) {
    if (!conn) {
        xsink->raiseException("NATS-PUBLISH-ERROR", "not connected");
        return -1;
    }

    natsMsg* msg = nullptr;
    natsStatus s = natsMsg_Create(&msg, subject, nullptr, (const char*)data, data_len);
    if (s != NATS_OK) {
        nats_error(xsink, "NATS-PUBLISH-ERROR", s, "failed to create message");
        return -1;
    }
    NatsMsgHolder holder(msg);

    // Set headers
    if (headers) {
        ConstHashIterator hi(headers);
        while (hi.next()) {
            QoreValue val = hi.get();
            if (val.getType() == NT_STRING) {
                s = natsMsgHeader_Set(msg, hi.getKey(), val.get<const QoreStringNode>()->c_str());
                if (s != NATS_OK) {
                    nats_error(xsink, "NATS-PUBLISH-ERROR", s,
                        "failed to set header '%s'", hi.getKey());
                    return -1;
                }
            }
        }
    }

    s = natsConnection_PublishMsg(conn, msg);
    if (s != NATS_OK) {
        nats_error(xsink, "NATS-PUBLISH-ERROR", s,
            "failed to publish message to subject '%s'", subject);
        return -1;
    }
    return 0;
}

QoreHashNode* QoreNatsConnection::request(const char* subject, const void* data,
        int data_len, int64 timeout_ms, ExceptionSink* xsink) {
    if (!conn) {
        xsink->raiseException("NATS-REQUEST-ERROR", "not connected");
        return nullptr;
    }

    natsMsg* reply = nullptr;
    natsStatus s = natsConnection_Request(&reply, conn, subject,
        data, data_len, timeout_ms);
    if (s == NATS_TIMEOUT) {
        xsink->raiseException("NATS-TIMEOUT-ERROR",
            "request to subject '%s' timed out after %lld ms", subject, timeout_ms);
        return nullptr;
    }
    if (s == NATS_NO_RESPONDERS) {
        xsink->raiseException("NATS-TIMEOUT-ERROR",
            "request to subject '%s': no responders available", subject);
        return nullptr;
    }
    if (s != NATS_OK) {
        nats_error(xsink, "NATS-REQUEST-ERROR", s,
            "request to subject '%s' failed", subject);
        return nullptr;
    }

    NatsMsgHolder holder(reply);
    return nats_msg_to_hash(reply, xsink);
}

QoreNatsSubscription* QoreNatsConnection::subscribe(const char* subject,
        ExceptionSink* xsink) {
    if (!conn) {
        xsink->raiseException("NATS-SUBSCRIBE-ERROR", "not connected");
        return nullptr;
    }

    natsSubscription* sub = nullptr;
    natsStatus s = natsConnection_SubscribeSync(&sub, conn, subject);
    if (s != NATS_OK) {
        nats_error(xsink, "NATS-SUBSCRIBE-ERROR", s,
            "failed to subscribe to subject '%s'", subject);
        return nullptr;
    }
    return new QoreNatsSubscription(sub);
}

QoreNatsSubscription* QoreNatsConnection::queueSubscribe(const char* subject,
        const char* queue, ExceptionSink* xsink) {
    if (!conn) {
        xsink->raiseException("NATS-SUBSCRIBE-ERROR", "not connected");
        return nullptr;
    }

    natsSubscription* sub = nullptr;
    natsStatus s = natsConnection_QueueSubscribeSync(&sub, conn, subject, queue);
    if (s != NATS_OK) {
        nats_error(xsink, "NATS-SUBSCRIBE-ERROR", s,
            "failed to queue subscribe to subject '%s' queue '%s'", subject, queue);
        return nullptr;
    }
    return new QoreNatsSubscription(sub);
}

QoreNatsJetStream* QoreNatsConnection::jetStream(ExceptionSink* xsink) {
    if (!conn) {
        xsink->raiseException("NATS-JETSTREAM-ERROR", "not connected");
        return nullptr;
    }
    QoreNatsJetStream* rv = new QoreNatsJetStream(conn, xsink);
    if (*xsink) {
        rv->deref(xsink);
        return nullptr;
    }
    return rv;
}

int QoreNatsConnection::drain(ExceptionSink* xsink) {
    if (!conn) {
        xsink->raiseException("NATS-CONNECTION-ERROR", "not connected");
        return -1;
    }
    natsStatus s = natsConnection_Drain(conn);
    if (s != NATS_OK) {
        nats_error(xsink, "NATS-CONNECTION-ERROR", s, "failed to drain connection");
        return -1;
    }
    return 0;
}

void QoreNatsConnection::close() {
    if (conn) {
        natsConnection_Close(conn);
    }
}

int QoreNatsConnection::flush(int64 timeout_ms, ExceptionSink* xsink) {
    if (!conn) {
        xsink->raiseException("NATS-CONNECTION-ERROR", "not connected");
        return -1;
    }
    natsStatus s = natsConnection_FlushTimeout(conn, timeout_ms);
    if (s != NATS_OK) {
        nats_error(xsink, "NATS-CONNECTION-ERROR", s, "failed to flush connection");
        return -1;
    }
    return 0;
}

bool QoreNatsConnection::isConnected() const {
    if (!conn) {
        return false;
    }
    return !natsConnection_IsClosed(conn);
}

int QoreNatsConnection::status() const {
    if (!conn) {
        return NATS_CONN_STATUS_CLOSED;
    }
    return natsConnection_Status(conn);
}
