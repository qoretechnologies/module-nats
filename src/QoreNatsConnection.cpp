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

QoreNatsConnection::QoreNatsConnection(const QoreHashNode* options, QoreProgram* pgm,
        ExceptionSink* xsink) {
    natsStatus s = natsOptions_Create(&opts);
    if (s != NATS_OK) {
        nats_error(xsink, "NATS-CONNECTION-ERROR", s, "failed to create NATS options");
        return;
    }

    if (configureOptions(options, xsink)) {
        return;
    }

    // Setup callbacks (must be done before connect, requires QoreProgram)
    if (setupCallbacks(options, pgm, xsink)) {
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
    if (cb_ctx) {
        ExceptionSink xsink;
        cb_ctx->cleanup(&xsink);
        delete cb_ctx;
        cb_ctx = nullptr;
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
        if (check_nats_file_access(v.get<const QoreStringNode>()->c_str(), xsink)) {
            return -1;
        }
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
        if (check_nats_file_access(v.get<const QoreStringNode>()->c_str(), xsink)) {
            return -1;
        }
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

    // Multiple server URLs
    v = options->getKeyValue("servers");
    if (v.getType() == NT_LIST) {
        const QoreListNode* servers = v.get<const QoreListNode>();
        int count = (int)servers->size();
        if (count > 0) {
            const char** srv_arr = (const char**)malloc(sizeof(const char*) * count);
            if (!srv_arr) {
                xsink->raiseException("NATS-CONNECTION-ERROR",
                    "memory allocation failed for %d servers", count);
                return -1;
            }
            for (int i = 0; i < count; ++i) {
                QoreValue sv = servers->retrieveEntry(i);
                srv_arr[i] = sv.getType() == NT_STRING
                    ? sv.get<const QoreStringNode>()->c_str() : "";
            }
            // Sandbox check each server URL
            for (int i = 0; i < count; ++i) {
                if (srv_arr[i][0] && check_nats_network_access(srv_arr[i], xsink)) {
                    free(srv_arr);
                    return -1;
                }
            }
            s = natsOptions_SetServers(opts, srv_arr, count);
            free(srv_arr);
            if (s != NATS_OK) {
                nats_error(xsink, "NATS-CONNECTION-ERROR", s, "failed to set servers");
                return -1;
            }
        }
    }

    // No echo
    v = options->getKeyValue("no_echo");
    if (v.getType() == NT_BOOLEAN) {
        s = natsOptions_SetNoEcho(opts, v.getAsBool());
        if (s != NATS_OK) {
            nats_error(xsink, "NATS-CONNECTION-ERROR", s, "failed to set no echo");
            return -1;
        }
    }

    // Send as soon as possible (low-latency mode)
    v = options->getKeyValue("send_asap");
    if (v.getType() == NT_BOOLEAN) {
        s = natsOptions_SetSendAsap(opts, v.getAsBool());
        if (s != NATS_OK) {
            nats_error(xsink, "NATS-CONNECTION-ERROR", s, "failed to set send asap");
            return -1;
        }
    }

    // Allow reconnect
    v = options->getKeyValue("allow_reconnect");
    if (v.getType() == NT_BOOLEAN) {
        s = natsOptions_SetAllowReconnect(opts, v.getAsBool());
        if (s != NATS_OK) {
            nats_error(xsink, "NATS-CONNECTION-ERROR", s, "failed to set allow reconnect");
            return -1;
        }
    }

    // Reconnect jitter
    {
        QoreValue jitter = options->getKeyValue("reconnect_jitter_ms");
        QoreValue jitterTls = options->getKeyValue("reconnect_jitter_tls_ms");
        if (jitter.getType() == NT_INT || jitterTls.getType() == NT_INT) {
            s = natsOptions_SetReconnectJitter(opts,
                jitter.getType() == NT_INT ? jitter.getAsBigInt() : 0,
                jitterTls.getType() == NT_INT ? jitterTls.getAsBigInt() : 0);
            if (s != NATS_OK) {
                nats_error(xsink, "NATS-CONNECTION-ERROR", s,
                    "failed to set reconnect jitter");
                return -1;
            }
        }
    }

    // Reconnect buffer size
    v = options->getKeyValue("reconnect_buf_size");
    if (v.getType() == NT_INT) {
        s = natsOptions_SetReconnectBufSize(opts, (int)v.getAsBigInt());
        if (s != NATS_OK) {
            nats_error(xsink, "NATS-CONNECTION-ERROR", s,
                "failed to set reconnect buffer size");
            return -1;
        }
    }

    // Max pending messages
    v = options->getKeyValue("max_pending_msgs");
    if (v.getType() == NT_INT) {
        s = natsOptions_SetMaxPendingMsgs(opts, (int)v.getAsBigInt());
        if (s != NATS_OK) {
            nats_error(xsink, "NATS-CONNECTION-ERROR", s,
                "failed to set max pending messages");
            return -1;
        }
    }

    // Max pending bytes
    v = options->getKeyValue("max_pending_bytes");
    if (v.getType() == NT_INT) {
        s = natsOptions_SetMaxPendingBytes(opts, v.getAsBigInt());
        if (s != NATS_OK) {
            nats_error(xsink, "NATS-CONNECTION-ERROR", s,
                "failed to set max pending bytes");
            return -1;
        }
    }

    // Fail requests on disconnect
    v = options->getKeyValue("fail_requests_on_disconnect");
    if (v.getType() == NT_BOOLEAN) {
        s = natsOptions_SetFailRequestsOnDisconnect(opts, v.getAsBool());
        if (s != NATS_OK) {
            nats_error(xsink, "NATS-CONNECTION-ERROR", s,
                "failed to set fail requests on disconnect");
            return -1;
        }
    }

    // IP resolution order
    v = options->getKeyValue("ip_resolution_order");
    if (v.getType() == NT_INT) {
        s = natsOptions_IPResolutionOrder(opts, (int)v.getAsBigInt());
        if (s != NATS_OK) {
            nats_error(xsink, "NATS-CONNECTION-ERROR", s,
                "failed to set IP resolution order");
            return -1;
        }
    }

    // Write deadline
    v = options->getKeyValue("write_deadline_ms");
    if (v.getType() == NT_INT) {
        s = natsOptions_SetWriteDeadline(opts, v.getAsBigInt());
        if (s != NATS_OK) {
            nats_error(xsink, "NATS-CONNECTION-ERROR", s,
                "failed to set write deadline");
            return -1;
        }
    }

    // Disable no responders
    v = options->getKeyValue("disable_no_responders");
    if (v.getType() == NT_BOOLEAN) {
        s = natsOptions_DisableNoResponders(opts, v.getAsBool());
        if (s != NATS_OK) {
            nats_error(xsink, "NATS-CONNECTION-ERROR", s,
                "failed to disable no responders");
            return -1;
        }
    }

    // Custom inbox prefix
    v = options->getKeyValue("custom_inbox_prefix");
    if (v.getType() == NT_STRING) {
        s = natsOptions_SetCustomInboxPrefix(opts, v.get<const QoreStringNode>()->c_str());
        if (s != NATS_OK) {
            nats_error(xsink, "NATS-CONNECTION-ERROR", s,
                "failed to set custom inbox prefix");
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
            if (check_nats_file_access(ca.get<const QoreStringNode>()->c_str(), xsink)) {
                return -1;
            }
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
            if (check_nats_file_access(cert.get<const QoreStringNode>()->c_str(), xsink)) {
                return -1;
            }
            if (check_nats_file_access(key.get<const QoreStringNode>()->c_str(), xsink)) {
                return -1;
            }
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

        // TLS cipher list (TLSv1.2 and below)
        QoreValue ciphers = tls->getKeyValue("ciphers");
        if (ciphers.getType() == NT_STRING) {
            s = natsOptions_SetCiphers(opts, ciphers.get<const QoreStringNode>()->c_str());
            if (s != NATS_OK) {
                nats_error(xsink, "NATS-TLS-ERROR", s, "failed to set TLS ciphers");
                return -1;
            }
        }

        // TLS cipher suites (TLSv1.3)
        QoreValue suites = tls->getKeyValue("cipher_suites");
        if (suites.getType() == NT_STRING) {
            s = natsOptions_SetCipherSuites(opts, suites.get<const QoreStringNode>()->c_str());
            if (s != NATS_OK) {
                nats_error(xsink, "NATS-TLS-ERROR", s, "failed to set TLS cipher suites");
                return -1;
            }
        }

        // Expected hostname in server certificate
        QoreValue hostname = tls->getKeyValue("expected_hostname");
        if (hostname.getType() == NT_STRING) {
            s = natsOptions_SetExpectedHostname(opts,
                hostname.get<const QoreStringNode>()->c_str());
            if (s != NATS_OK) {
                nats_error(xsink, "NATS-TLS-ERROR", s,
                    "failed to set expected hostname");
                return -1;
            }
        }

        // TLS handshake first
        QoreValue hsfirst = tls->getKeyValue("tls_handshake_first");
        if (hsfirst.getType() == NT_BOOLEAN && hsfirst.getAsBool()) {
            s = natsOptions_TLSHandshakeFirst(opts);
            if (s != NATS_OK) {
                nats_error(xsink, "NATS-TLS-ERROR", s,
                    "failed to set TLS handshake first");
                return -1;
            }
        }
    }

    return 0;
}

// Helper to extract a ResolvedCallReferenceNode from a hash value
static ResolvedCallReferenceNode* extract_callback(const QoreHashNode* opts,
        const char* key) {
    QoreValue v = opts->getKeyValue(key);
    if (v.getType() == NT_FUNCREF || v.getType() == NT_RUNTIME_CLOSURE) {
        return dynamic_cast<ResolvedCallReferenceNode*>(v.getInternalNode());
    }
    return nullptr;
}

int QoreNatsConnection::setupCallbacks(const QoreHashNode* options, QoreProgram* pgm,
        ExceptionSink* xsink) {
    // Check if any callbacks are provided
    ResolvedCallReferenceNode* disc = extract_callback(options, "on_disconnect");
    ResolvedCallReferenceNode* recon = extract_callback(options, "on_reconnect");
    ResolvedCallReferenceNode* closed = extract_callback(options, "on_closed");
    ResolvedCallReferenceNode* err = extract_callback(options, "on_error");
    ResolvedCallReferenceNode* lame = extract_callback(options, "on_lame_duck");
    ResolvedCallReferenceNode* discovered = extract_callback(options, "on_discovered_servers");

    if (!disc && !recon && !closed && !err && !lame && !discovered) {
        return 0;  // No callbacks
    }

    // Create callback context
    cb_ctx = new NatsCallbackContext();
    cb_ctx->pgm = pgm;
    if (cb_ctx->pgm) {
        cb_ctx->pgm->ref();
    }

    natsStatus s;

    if (disc) {
        cb_ctx->on_disconnect = disc->refRefSelf();
        s = natsOptions_SetDisconnectedCB(opts, disconnectHandler, cb_ctx);
        if (s != NATS_OK) {
            nats_error(xsink, "NATS-CONNECTION-ERROR", s,
                "failed to set disconnect callback");
            return -1;
        }
    }

    if (recon) {
        cb_ctx->on_reconnect = recon->refRefSelf();
        s = natsOptions_SetReconnectedCB(opts, reconnectHandler, cb_ctx);
        if (s != NATS_OK) {
            nats_error(xsink, "NATS-CONNECTION-ERROR", s,
                "failed to set reconnect callback");
            return -1;
        }
    }

    if (closed) {
        cb_ctx->on_closed = closed->refRefSelf();
        s = natsOptions_SetClosedCB(opts, closedHandler, cb_ctx);
        if (s != NATS_OK) {
            nats_error(xsink, "NATS-CONNECTION-ERROR", s,
                "failed to set closed callback");
            return -1;
        }
    }

    if (err) {
        cb_ctx->on_error = err->refRefSelf();
        s = natsOptions_SetErrorHandler(opts, errorHandler, cb_ctx);
        if (s != NATS_OK) {
            nats_error(xsink, "NATS-CONNECTION-ERROR", s,
                "failed to set error handler");
            return -1;
        }
    }

    if (lame) {
        cb_ctx->on_lame_duck = lame->refRefSelf();
        s = natsOptions_SetLameDuckModeCB(opts, lameDuckHandler, cb_ctx);
        if (s != NATS_OK) {
            nats_error(xsink, "NATS-CONNECTION-ERROR", s,
                "failed to set lame duck callback");
            return -1;
        }
    }

    if (discovered) {
        cb_ctx->on_discovered_servers = discovered->refRefSelf();
        s = natsOptions_SetDiscoveredServersCB(opts, discoveredServersHandler, cb_ctx);
        if (s != NATS_OK) {
            nats_error(xsink, "NATS-CONNECTION-ERROR", s,
                "failed to set discovered servers callback");
            return -1;
        }
    }

    return 0;
}

void QoreNatsConnection::execCallback(ResolvedCallReferenceNode* cb,
        QoreProgram* pgm, QoreListNode* args) {
    if (!cb || !pgm) {
        return;
    }
    // Register this (nats.c) thread with the Qore runtime
    QoreForeignThreadHelper fth;

    ExceptionSink xsink;
    QoreExternalProgramContextHelper pch(&xsink, pgm);
    if (!xsink) {
        if (args) {
            cb->execValue(args, &xsink).discard(&xsink);
        } else {
            ReferenceHolder<QoreListNode> empty_args(new QoreListNode(autoTypeInfo), &xsink);
            cb->execValue(*empty_args, &xsink).discard(&xsink);
        }
    }
    // Exceptions from callbacks cannot be propagated - just clear them
    if (xsink) {
        xsink.clear();
    }
}

void QoreNatsConnection::disconnectHandler(natsConnection* nc, void* closure) {
    NatsCallbackContext* ctx = static_cast<NatsCallbackContext*>(closure);
    execCallback(ctx->on_disconnect, ctx->pgm);
}

void QoreNatsConnection::reconnectHandler(natsConnection* nc, void* closure) {
    NatsCallbackContext* ctx = static_cast<NatsCallbackContext*>(closure);
    execCallback(ctx->on_reconnect, ctx->pgm);
}

void QoreNatsConnection::closedHandler(natsConnection* nc, void* closure) {
    NatsCallbackContext* ctx = static_cast<NatsCallbackContext*>(closure);
    execCallback(ctx->on_closed, ctx->pgm);
}

void QoreNatsConnection::errorHandler(natsConnection* nc, natsSubscription* sub,
        natsStatus err, void* closure) {
    NatsCallbackContext* ctx = static_cast<NatsCallbackContext*>(closure);
    if (!ctx->on_error || !ctx->pgm) {
        return;
    }
    QoreForeignThreadHelper fth;

    ExceptionSink xsink;
    QoreExternalProgramContextHelper pch(&xsink, ctx->pgm);
    if (!xsink) {
        ReferenceHolder<QoreListNode> args(new QoreListNode(autoTypeInfo), &xsink);
        args->push(new QoreStringNode(natsStatus_GetText(err)), &xsink);
        if (!xsink) {
            ctx->on_error->execValue(*args, &xsink).discard(&xsink);
        }
    }
    if (xsink) {
        xsink.clear();
    }
}

void QoreNatsConnection::lameDuckHandler(natsConnection* nc, void* closure) {
    NatsCallbackContext* ctx = static_cast<NatsCallbackContext*>(closure);
    execCallback(ctx->on_lame_duck, ctx->pgm);
}

void QoreNatsConnection::discoveredServersHandler(natsConnection* nc, void* closure) {
    NatsCallbackContext* ctx = static_cast<NatsCallbackContext*>(closure);
    execCallback(ctx->on_discovered_servers, ctx->pgm);
}

int QoreNatsConnection::publish(const char* subject, const void* data, int data_len,
        ExceptionSink* xsink) {
    if (!conn) {
        xsink->raiseException("NATS-PUBLISH-ERROR", "not connected");
        return -1;
    }
    if (qore_check_cancel(xsink)) {
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
    if (qore_check_cancel(xsink)) {
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

        natsMsg* reply = nullptr;
        natsStatus s = natsConnection_Request(&reply, conn, subject,
            data, data_len, effective_timeout);

        if (s == NATS_OK) {
            NatsMsgHolder holder(reply);
            return nats_msg_to_hash(reply, xsink);
        }

        if (s == NATS_NO_RESPONDERS) {
            xsink->raiseException("NATS-TIMEOUT-ERROR",
                "request to subject '%s': no responders available", subject);
            return nullptr;
        }

        if (s == NATS_TIMEOUT) {
            remaining_ms -= effective_timeout;
            if (remaining_ms <= 0) {
                xsink->raiseException("NATS-TIMEOUT-ERROR",
                    "request to subject '%s' timed out after %lld ms",
                    subject, timeout_ms);
                return nullptr;
            }
            continue;
        }

        nats_error(xsink, "NATS-REQUEST-ERROR", s,
            "request to subject '%s' failed", subject);
        return nullptr;
    }
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
    if (qore_check_cancel(xsink)) {
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

        natsStatus s = natsConnection_FlushTimeout(conn, effective_timeout);
        if (s == NATS_OK) {
            return 0;
        }

        if (s == NATS_TIMEOUT) {
            remaining_ms -= effective_timeout;
            if (remaining_ms <= 0) {
                nats_error(xsink, "NATS-CONNECTION-ERROR", s,
                    "failed to flush connection");
                return -1;
            }
            continue;
        }

        nats_error(xsink, "NATS-CONNECTION-ERROR", s, "failed to flush connection");
        return -1;
    }
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

QoreStringNode* QoreNatsConnection::getConnectedUrl(ExceptionSink* xsink) {
    if (!conn) {
        xsink->raiseException("NATS-CONNECTION-ERROR", "not connected");
        return nullptr;
    }
    char buf[512];
    natsStatus s = natsConnection_GetConnectedUrl(conn, buf, sizeof(buf));
    if (s != NATS_OK) {
        nats_error(xsink, "NATS-CONNECTION-ERROR", s, "failed to get connected URL");
        return nullptr;
    }
    return new QoreStringNode(buf);
}

QoreStringNode* QoreNatsConnection::getServerId(ExceptionSink* xsink) {
    if (!conn) {
        xsink->raiseException("NATS-CONNECTION-ERROR", "not connected");
        return nullptr;
    }
    char buf[256];
    natsStatus s = natsConnection_GetConnectedServerId(conn, buf, sizeof(buf));
    if (s != NATS_OK) {
        return nullptr;
    }
    return new QoreStringNode(buf);
}

int64 QoreNatsConnection::getClientId(ExceptionSink* xsink) {
    if (!conn) {
        xsink->raiseException("NATS-CONNECTION-ERROR", "not connected");
        return -1;
    }
    uint64_t cid = 0;
    natsStatus s = natsConnection_GetClientID(conn, &cid);
    if (s != NATS_OK) {
        nats_error(xsink, "NATS-CONNECTION-ERROR", s, "failed to get client ID");
        return -1;
    }
    return (int64)cid;
}

QoreStringNode* QoreNatsConnection::getClientIp(ExceptionSink* xsink) {
    if (!conn) {
        xsink->raiseException("NATS-CONNECTION-ERROR", "not connected");
        return nullptr;
    }
    char* ip = nullptr;
    natsStatus s = natsConnection_GetClientIP(conn, &ip);
    if (s != NATS_OK || !ip) {
        return nullptr;
    }
    QoreStringNode* rv = new QoreStringNode(ip);
    free(ip);
    return rv;
}

QoreValue QoreNatsConnection::getRtt(ExceptionSink* xsink) {
    if (!conn) {
        xsink->raiseException("NATS-CONNECTION-ERROR", "not connected");
        return QoreValue();
    }
    int64_t rtt_ns = 0;
    natsStatus s = natsConnection_GetRTT(conn, &rtt_ns);
    if (s != NATS_OK) {
        return QoreValue();
    }
    return QoreValue(rtt_ns / 1000000LL);
}

int64 QoreNatsConnection::getMaxPayload(ExceptionSink* xsink) {
    if (!conn) {
        xsink->raiseException("NATS-CONNECTION-ERROR", "not connected");
        return -1;
    }
    return (int64)natsConnection_GetMaxPayload(conn);
}

bool QoreNatsConnection::hasHeaderSupport(ExceptionSink* xsink) {
    if (!conn) {
        xsink->raiseException("NATS-CONNECTION-ERROR", "not connected");
        return false;
    }
    return natsConnection_HasHeaderSupport(conn) == NATS_OK;
}

QoreHashNode* QoreNatsConnection::getLocalIpAndPort(ExceptionSink* xsink) {
    if (!conn) {
        xsink->raiseException("NATS-CONNECTION-ERROR", "not connected");
        return nullptr;
    }
    char* ip = nullptr;
    int port = 0;
    natsStatus s = natsConnection_GetLocalIPAndPort(conn, &ip, &port);
    if (s != NATS_OK) {
        nats_error(xsink, "NATS-CONNECTION-ERROR", s,
            "failed to get local IP and port");
        return nullptr;
    }
    ReferenceHolder<QoreHashNode> h(new QoreHashNode(autoTypeInfo), xsink);
    if (ip) {
        h->setKeyValue("ip", new QoreStringNode(ip), xsink);
        free(ip);
        if (*xsink) {
            return nullptr;
        }
    }
    if (!*xsink) {
        h->setKeyValue("port", (int64)port, xsink);
    }
    if (*xsink) {
        return nullptr;
    }
    return h.release();
}

bool QoreNatsConnection::isReconnecting() const {
    if (!conn) {
        return false;
    }
    return natsConnection_IsReconnecting(conn);
}

bool QoreNatsConnection::isDraining() const {
    if (!conn) {
        return false;
    }
    return natsConnection_IsDraining(conn);
}

int QoreNatsConnection::buffered() const {
    if (!conn) {
        return 0;
    }
    return natsConnection_Buffered(conn);
}

QoreHashNode* QoreNatsConnection::getStats(ExceptionSink* xsink) {
    if (!conn) {
        xsink->raiseException("NATS-CONNECTION-ERROR", "not connected");
        return nullptr;
    }
    if (qore_check_cancel(xsink)) {
        return nullptr;
    }
    natsStatistics* stats = nullptr;
    natsStatus s = natsStatistics_Create(&stats);
    if (s != NATS_OK) {
        nats_error(xsink, "NATS-CONNECTION-ERROR", s, "failed to create statistics");
        return nullptr;
    }

    s = natsConnection_GetStats(conn, stats);
    if (s != NATS_OK) {
        natsStatistics_Destroy(stats);
        nats_error(xsink, "NATS-CONNECTION-ERROR", s, "failed to get statistics");
        return nullptr;
    }

    uint64_t inMsgs = 0, inBytes = 0, outMsgs = 0, outBytes = 0, reconnects = 0;
    s = natsStatistics_GetCounts(stats, &inMsgs, &inBytes, &outMsgs, &outBytes, &reconnects);
    natsStatistics_Destroy(stats);
    if (s != NATS_OK) {
        nats_error(xsink, "NATS-CONNECTION-ERROR", s, "failed to get statistics counts");
        return nullptr;
    }

    ReferenceHolder<QoreHashNode> h(new QoreHashNode(hashdeclNatsConnectionStats, xsink), xsink);
    if (*xsink) {
        return nullptr;
    }
    h->setKeyValue("in_msgs", (int64)inMsgs, xsink);
    if (!*xsink) {
        h->setKeyValue("in_bytes", (int64)inBytes, xsink);
    }
    if (!*xsink) {
        h->setKeyValue("out_msgs", (int64)outMsgs, xsink);
    }
    if (!*xsink) {
        h->setKeyValue("out_bytes", (int64)outBytes, xsink);
    }
    if (!*xsink) {
        h->setKeyValue("reconnects", (int64)reconnects, xsink);
    }
    if (*xsink) {
        return nullptr;
    }
    return h.release();
}

static QoreListNode* nats_server_list_to_qore(char** servers, int count, ExceptionSink* xsink) {
    ReferenceHolder<QoreListNode> list(new QoreListNode(stringTypeInfo), xsink);
    for (int i = 0; i < count; ++i) {
        list->push(new QoreStringNode(servers[i]), xsink);
        free(servers[i]);
        if (*xsink) {
            // Free remaining strings
            for (int j = i + 1; j < count; ++j) {
                free(servers[j]);
            }
            free(servers);
            return nullptr;
        }
    }
    free(servers);
    return list.release();
}

QoreListNode* QoreNatsConnection::getServers(ExceptionSink* xsink) {
    if (!conn) {
        xsink->raiseException("NATS-CONNECTION-ERROR", "not connected");
        return nullptr;
    }
    if (qore_check_cancel(xsink)) {
        return nullptr;
    }
    char** servers = nullptr;
    int count = 0;
    natsStatus s = natsConnection_GetServers(conn, &servers, &count);
    if (s != NATS_OK) {
        nats_error(xsink, "NATS-CONNECTION-ERROR", s, "failed to get servers");
        return nullptr;
    }
    if (!servers || count == 0) {
        if (servers) {
            free(servers);
        }
        return new QoreListNode(stringTypeInfo);
    }
    return nats_server_list_to_qore(servers, count, xsink);
}

QoreListNode* QoreNatsConnection::getDiscoveredServers(ExceptionSink* xsink) {
    if (!conn) {
        xsink->raiseException("NATS-CONNECTION-ERROR", "not connected");
        return nullptr;
    }
    if (qore_check_cancel(xsink)) {
        return nullptr;
    }
    char** servers = nullptr;
    int count = 0;
    natsStatus s = natsConnection_GetDiscoveredServers(conn, &servers, &count);
    if (s != NATS_OK) {
        nats_error(xsink, "NATS-CONNECTION-ERROR", s, "failed to get discovered servers");
        return nullptr;
    }
    if (!servers || count == 0) {
        if (servers) {
            free(servers);
        }
        return new QoreListNode(stringTypeInfo);
    }
    return nats_server_list_to_qore(servers, count, xsink);
}

int QoreNatsConnection::reconnect(ExceptionSink* xsink) {
    if (!conn) {
        xsink->raiseException("NATS-CONNECTION-ERROR", "not connected");
        return -1;
    }
    if (qore_check_cancel(xsink)) {
        return -1;
    }
    natsStatus s = natsConnection_Reconnect(conn);
    if (s != NATS_OK) {
        nats_error(xsink, "NATS-CONNECTION-ERROR", s, "failed to reconnect");
        return -1;
    }
    return 0;
}

QoreHashNode* QoreNatsConnection::getConnectionInfo(ExceptionSink* xsink) {
    if (!conn) {
        xsink->raiseException("NATS-CONNECTION-ERROR", "not connected");
        return nullptr;
    }
    if (qore_check_cancel(xsink)) {
        return nullptr;
    }

    ReferenceHolder<QoreHashNode> h(new QoreHashNode(hashdeclNatsConnectionInfo, xsink), xsink);
    if (*xsink) {
        return nullptr;
    }

    // connected_url
    char url_buf[512];
    if (natsConnection_GetConnectedUrl(conn, url_buf, sizeof(url_buf)) == NATS_OK) {
        h->setKeyValue("connected_url", new QoreStringNode(url_buf), xsink);
        if (*xsink) {
            return nullptr;
        }
    }

    // server_id
    char id_buf[256];
    if (natsConnection_GetConnectedServerId(conn, id_buf, sizeof(id_buf)) == NATS_OK) {
        h->setKeyValue("server_id", new QoreStringNode(id_buf), xsink);
        if (*xsink) {
            return nullptr;
        }
    }

    // client_id
    uint64_t cid = 0;
    if (natsConnection_GetClientID(conn, &cid) == NATS_OK) {
        h->setKeyValue("client_id", (int64)cid, xsink);
        if (*xsink) {
            return nullptr;
        }
    }

    // client_ip
    {
        char* ip = nullptr;
        if (natsConnection_GetClientIP(conn, &ip) == NATS_OK && ip) {
            h->setKeyValue("client_ip", new QoreStringNode(ip), xsink);
            free(ip);
            if (*xsink) {
                return nullptr;
            }
        }
    }

    // rtt_ms
    {
        int64_t rtt_ns = 0;
        if (natsConnection_GetRTT(conn, &rtt_ns) == NATS_OK) {
            h->setKeyValue("rtt_ms", (int64)(rtt_ns / 1000000LL), xsink);
            if (*xsink) {
                return nullptr;
            }
        }
    }

    // max_payload
    if (!*xsink) {
        h->setKeyValue("max_payload", (int64)natsConnection_GetMaxPayload(conn), xsink);
    }

    // has_header_support
    if (!*xsink) {
        h->setKeyValue("has_header_support",
            natsConnection_HasHeaderSupport(conn) == NATS_OK, xsink);
    }

    // local_ip + local_port
    {
        char* lip = nullptr;
        int lport = 0;
        if (natsConnection_GetLocalIPAndPort(conn, &lip, &lport) == NATS_OK) {
            if (lip) {
                h->setKeyValue("local_ip", new QoreStringNode(lip), xsink);
                free(lip);
                if (*xsink) {
                    return nullptr;
                }
            }
            if (!*xsink) {
                h->setKeyValue("local_port", (int64)lport, xsink);
            }
        }
    }

    // is_reconnecting
    if (!*xsink) {
        h->setKeyValue("is_reconnecting", natsConnection_IsReconnecting(conn), xsink);
    }

    // is_draining
    if (!*xsink) {
        h->setKeyValue("is_draining", natsConnection_IsDraining(conn), xsink);
    }

    // buffered
    if (!*xsink) {
        h->setKeyValue("buffered", (int64)natsConnection_Buffered(conn), xsink);
    }

    if (*xsink) {
        return nullptr;
    }

    // servers
    {
        char** srvs = nullptr;
        int cnt = 0;
        if (natsConnection_GetServers(conn, &srvs, &cnt) == NATS_OK && srvs) {
            if (cnt > 0) {
                QoreListNode* srv_list = nats_server_list_to_qore(srvs, cnt, xsink);
                if (*xsink) {
                    return nullptr;
                }
                h->setKeyValue("servers", srv_list, xsink);
                if (*xsink) {
                    return nullptr;
                }
            } else {
                free(srvs);
            }
        }
    }

    // discovered_servers
    {
        char** dsrvs = nullptr;
        int dcnt = 0;
        if (natsConnection_GetDiscoveredServers(conn, &dsrvs, &dcnt) == NATS_OK && dsrvs) {
            if (dcnt > 0) {
                QoreListNode* dsrv_list = nats_server_list_to_qore(dsrvs, dcnt, xsink);
                if (*xsink) {
                    return nullptr;
                }
                h->setKeyValue("discovered_servers", dsrv_list, xsink);
                if (*xsink) {
                    return nullptr;
                }
            } else {
                free(dsrvs);
            }
        }
    }

    // stats
    {
        QoreHashNode* stats = getStats(xsink);
        if (*xsink) {
            return nullptr;
        }
        h->setKeyValue("stats", stats, xsink);
        if (*xsink) {
            return nullptr;
        }
    }

    return h.release();
}
