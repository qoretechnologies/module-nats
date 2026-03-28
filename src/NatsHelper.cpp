/* -*- mode: c++; indent-tabs-mode: nil -*- */
/** @file NatsHelper.cpp NATS helper function implementations */
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

#include "NatsHelper.h"

#include <cstring>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

void nats_error(ExceptionSink* xsink, const char* err, natsStatus s,
        const char* fmt, ...) {
    QoreString desc;
    while (true) {
        va_list args;
        va_start(args, fmt);
        int rc = desc.vsprintf(fmt, args);
        va_end(args);
        if (!rc) {
            break;
        }
    }
    desc.concat(": ");
    desc.concat(natsStatus_GetText(s));
    xsink->raiseException(err, desc.c_str());
}

void nats_js_error(ExceptionSink* xsink, const char* err, natsStatus s,
        jsErrCode jerr, const char* fmt, ...) {
    QoreString desc;
    while (true) {
        va_list args;
        va_start(args, fmt);
        int rc = desc.vsprintf(fmt, args);
        va_end(args);
        if (!rc) {
            break;
        }
    }
    desc.concat(": ");
    desc.concat(natsStatus_GetText(s));
    if (jerr != 0) {
        desc.sprintf(" (JetStream error code: %d)", (int)jerr);
    }
    xsink->raiseException(err, desc.c_str());
}

QoreHashNode* nats_msg_to_hash(natsMsg* msg, ExceptionSink* xsink) {
    ReferenceHolder<QoreHashNode> h(new QoreHashNode(hashdeclNatsMsgInfo, xsink), xsink);
    if (*xsink) {
        return nullptr;
    }

    // subject
    const char* subject = natsMsg_GetSubject(msg);
    if (subject) {
        h->setKeyValue("subject", new QoreStringNode(subject), xsink);
        if (*xsink) {
            return nullptr;
        }
    }

    // data as binary
    int data_len = natsMsg_GetDataLength(msg);
    const char* data = natsMsg_GetData(msg);
    if (data && data_len > 0) {
        BinaryNode* bin = new BinaryNode();
        bin->append(data, data_len);
        h->setKeyValue("data", bin, xsink);
        if (*xsink) {
            return nullptr;
        }
    }

    // reply subject
    const char* reply = natsMsg_GetReply(msg);
    if (reply) {
        h->setKeyValue("reply", new QoreStringNode(reply), xsink);
        if (*xsink) {
            return nullptr;
        }
    }

    // headers
    const char** keys = nullptr;
    int num_keys = 0;
    natsStatus s = natsMsgHeader_Keys(msg, &keys, &num_keys);
    if (s == NATS_OK && keys && num_keys > 0) {
        ReferenceHolder<QoreHashNode> headers(new QoreHashNode(stringTypeInfo), xsink);
        for (int i = 0; i < num_keys; ++i) {
            const char* value = nullptr;
            if (natsMsgHeader_Get(msg, keys[i], &value) == NATS_OK && value) {
                headers->setKeyValue(keys[i], new QoreStringNode(value), xsink);
                if (*xsink) {
                    free(keys);
                    return nullptr;
                }
            }
        }
        free(keys);
        h->setKeyValue("headers", headers.release(), xsink);
        if (*xsink) {
            return nullptr;
        }
    } else if (keys) {
        free(keys);
    }

    return h.release();
}

QoreHashNode* nats_kv_entry_to_hash(kvEntry* entry, ExceptionSink* xsink) {
    if (!entry) {
        xsink->raiseException("NATS-KV-ERROR", "internal error: null KV entry");
        return nullptr;
    }

    ReferenceHolder<QoreHashNode> h(new QoreHashNode(hashdeclNatsKVEntry, xsink), xsink);
    if (*xsink) {
        return nullptr;
    }

    const char* bucket = kvEntry_Bucket(entry);
    if (bucket) {
        h->setKeyValue("bucket", new QoreStringNode(bucket), xsink);
        if (*xsink) {
            return nullptr;
        }
    }

    const char* key = kvEntry_Key(entry);
    if (key) {
        h->setKeyValue("key", new QoreStringNode(key), xsink);
        if (*xsink) {
            return nullptr;
        }
    }

    const void* val = kvEntry_Value(entry);
    int val_len = kvEntry_ValueLen(entry);
    if (val && val_len > 0) {
        SimpleRefHolder<BinaryNode> bin(new BinaryNode);
        bin->append(val, val_len);
        h->setKeyValue("value", bin.release(), xsink);
        if (*xsink) {
            return nullptr;
        }
    }

    h->setKeyValue("revision", (int64)kvEntry_Revision(entry), xsink);
    if (*xsink) {
        return nullptr;
    }

    // Created timestamp (nanoseconds since epoch -> Qore date)
    int64 created_ns = kvEntry_Created(entry);
    if (created_ns > 0) {
        int64 created_us = created_ns / 1000;
        h->setKeyValue("created", DateTimeNode::makeAbsolute(
            currentTZ(), created_us / 1000000, (int)(created_us % 1000000)), xsink);
        if (*xsink) {
            return nullptr;
        }
    }

    h->setKeyValue("operation", (int64)kvEntry_Operation(entry), xsink);
    if (*xsink) {
        return nullptr;
    }

    return h.release();
}

//! Parse a NATS URL into host and port
/** Supports nats://host:port, tls://host:port, and host:port formats
*/
static int parse_nats_url(const char* url, QoreString& host, int& port) {
    const char* p = url;

    // Skip scheme
    if (strncmp(p, "nats://", 7) == 0) {
        p += 7;
    } else if (strncmp(p, "tls://", 6) == 0) {
        p += 6;
    }

    // Skip userinfo (user:password@)
    const char* at = strchr(p, '@');
    if (at) {
        p = at + 1;
    }

    // Parse host:port
    const char* colon = strrchr(p, ':');
    if (colon) {
        host.set(p, colon - p);
        port = atoi(colon + 1);
    } else {
        host.set(p);
        port = 4222;  // default NATS port
    }

    // Remove trailing slash
    if (host.size() > 0 && host[host.size() - 1] == '/') {
        host.terminate(host.size() - 1);
    }

    return 0;
}

int check_nats_network_access(const char* url, ExceptionSink* xsink) {
    QoreSandboxManagerHelper smh;
    if (!smh) {
        return 0;  // No sandbox manager = allow all
    }
    QoreSandboxManager* sm = smh.get();

    QoreString host;
    int port;
    parse_nats_url(url, host, port);

    if (host.empty()) {
        return 0;
    }

    // Check hostname policy before DNS resolution
    if (sm->network().checkHostname(host.c_str(), port, QSEC_NET_TCP)) {
        xsink->raiseException("NATS-CONNECTION-ERROR",
            "access to host '%s:%d' is not allowed by the network security policy",
            host.c_str(), port);
        return -1;
    }

    // Resolve hostname and check all addresses
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo* res = nullptr;
    int rv = getaddrinfo(host.c_str(), nullptr, &hints, &res);
    if (rv != 0) {
        xsink->raiseException("NATS-CONNECTION-ERROR",
            "cannot resolve hostname '%s': %s", host.c_str(), gai_strerror(rv));
        return -1;
    }

    bool denied = false;
    for (struct addrinfo* p = res; p; p = p->ai_next) {
        if (p->ai_family == AF_INET) {
            struct sockaddr_in* addr = (struct sockaddr_in*)p->ai_addr;
            addr->sin_port = htons(port);
        } else if (p->ai_family == AF_INET6) {
            struct sockaddr_in6* addr = (struct sockaddr_in6*)p->ai_addr;
            addr->sin6_port = htons(port);
        }
        if (sm->checkNetworkAccess(p->ai_addr, p->ai_addrlen, QSEC_NET_TCP, xsink)) {
            denied = true;
            break;
        }
    }
    freeaddrinfo(res);

    return denied ? -1 : 0;
}

int check_nats_file_access(const char* path, ExceptionSink* xsink) {
    QoreSandboxManagerHelper smh;
    if (!smh) {
        return 0;  // No sandbox manager = allow all
    }

    if (smh->checkFilesystemAccess(path, QSEC_READ, xsink)) {
        return -1;  // Access denied, exception already raised
    }

    return 0;
}
