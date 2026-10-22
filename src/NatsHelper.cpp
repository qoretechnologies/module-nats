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

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <random>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <vector>

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

void nats_micro_error(ExceptionSink* xsink, const char* err, microError* me,
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
    if (me) {
        char buf[256];
        microError_String(me, buf, sizeof(buf));
        desc.concat(": ");
        desc.concat(buf);
        microError_Destroy(me);
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
    // a policy check: under a policy barrier, the caller's network policy does not apply
    QoreSandboxManagerHelper smh(QoreSandboxManagerHelper::Policy);
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

    // Check hostname policy before DNS resolution.  checkHostname() returns true when access is
    // ALLOWED, so the policy denies the connection when it returns false.
    if (!sm->network().checkHostname(host.c_str(), port, QSEC_NET_TCP)) {
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
        // checkNetworkAccess() returns true when access is ALLOWED and false (raising an
        // exception) when denied, so a resolved address is blocked when it returns false.
        if (!sm->checkNetworkAccess(p->ai_addr, p->ai_addrlen, QSEC_NET_TCP, xsink)) {
            denied = true;
            break;
        }
    }
    freeaddrinfo(res);

    return denied ? -1 : 0;
}

int check_nats_file_access(const char* path, ExceptionSink* xsink) {
    // a policy check: under a policy barrier, the caller's filesystem policy does not apply
    QoreSandboxManagerHelper smh(QoreSandboxManagerHelper::Policy);
    if (!smh) {
        return 0;  // No sandbox manager = allow all
    }

    // checkFilesystemAccess() returns true when access is ALLOWED and false (raising an
    // exception) when denied, so access is blocked when it returns false.
    if (!smh->checkFilesystemAccess(path, QSEC_READ, xsink)) {
        return -1;  // Access denied, exception already raised
    }

    return 0;
}

namespace {
// the C client's default connect timeout (NATS_OPTS_DEFAULT_TIMEOUT)
constexpr int64 NATS_DEFAULT_CONNECT_TIMEOUT_MS = 2000;
// the shortest time given to each address of a host, as the C client does
constexpr int64 NATS_MIN_CONNECT_TIMEOUT_PER_ADDRESS_MS = 10;

// closes a socket unless it is released
class SocketHolder {
public:
    DLLLOCAL explicit SocketHolder(int fd) : fd(fd) {
    }

    DLLLOCAL ~SocketHolder() {
        if (fd >= 0) {
            ::close(fd);
        }
    }

    DLLLOCAL int release() {
        int rv = fd;
        fd = -1;
        return rv;
    }

private:
    int fd;
};

// frees an addrinfo list
struct AddrInfoFree {
    DLLLOCAL void operator()(struct addrinfo* ai) const {
        freeaddrinfo(ai);
    }
};
}

NatsSandboxDialer::NatsSandboxDialer(int64 timeout_ms, int order_ip, bool no_randomize)
        : timeout_ms(timeout_ms > 0 ? timeout_ms : NATS_DEFAULT_CONNECT_TIMEOUT_MS), order_ip(order_ip),
        no_randomize(no_randomize) {
}

natsStatus NatsSandboxDialer::connect(natsSock* fd, char* host, int port, void* closure) {
    // no C++ exception may leave the C client's callback
    try {
        return static_cast<NatsSandboxDialer*>(closure)->connectIntern(fd, host, port);
    } catch (const std::bad_alloc&) {
        return NATS_NO_MEMORY;
    } catch (...) {
        return NATS_ERR;
    }
}

void NatsSandboxDialer::takeException(ExceptionSink* xsink) {
    std::unique_ptr<ExceptionSink> e;
    {
        std::lock_guard<std::mutex> lock(m);
        initial = false;
        e = std::move(pending);
    }
    if (e) {
        xsink->assimilate(*e);
    }
}

void NatsSandboxDialer::keepException(ExceptionSink& xsink) {
    std::lock_guard<std::mutex> lock(m);
    if (initial && !pending) {
        pending.reset(new ExceptionSink);
        pending->assimilate(xsink);
        return;
    }
    // a reconnection on the C client's thread: nobody can receive the exception
    xsink.clear();
}

natsStatus NatsSandboxDialer::connectIntern(natsSock* fd, const char* host, int port) {
    // the reconnect thread of the C client is not a Qore thread; the first connection is made on the Qore thread that
    // creates the connection, where the helper does nothing
    QoreForeignThreadHelper fth;
    // the connection is made under the sandbox of the code that created the connection
    QoreSandboxCaptureHelper sch(sandbox);
    ExceptionSink xsink;

    // the address families to resolve, in the order of the IP resolution order option
    int families[2] = {AF_UNSPEC, AF_UNSPEC};
    int nfamilies = 1;
    switch (order_ip) {
        case 4: families[0] = AF_INET; break;
        case 6: families[0] = AF_INET6; break;
        case 46: families[0] = AF_INET; families[1] = AF_INET6; nfamilies = 2; break;
        case 64: families[0] = AF_INET6; families[1] = AF_INET; nfamilies = 2; break;
        default: break;
    }

    char sport[16];
    snprintf(sport, sizeof(sport), "%d", port);

    std::vector<std::unique_ptr<struct addrinfo, AddrInfoFree>> results;
    std::vector<const struct addrinfo*> addresses;
    for (int i = 0; i < nfamilies; ++i) {
        struct addrinfo hints;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = families[i];
        hints.ai_socktype = SOCK_STREAM;
        struct addrinfo* res = nullptr;
        if (getaddrinfo(host, sport, &hints, &res) || !res) {
            continue;
        }
        results.emplace_back(res);
        size_t start = addresses.size();
        for (const struct addrinfo* ai = res; ai; ai = ai->ai_next) {
            addresses.push_back(ai);
        }
        // the C client tries the addresses of a host in a random order unless no_randomize is set
        if (!no_randomize && addresses.size() - start > 1) {
            static thread_local std::mt19937 rng{std::random_device{}()};
            std::shuffle(addresses.begin() + start, addresses.end(), rng);
        }
    }
    if (addresses.empty()) {
        return NATS_NO_SERVER;
    }

    int64 per_address_ms = timeout_ms / static_cast<int64>(addresses.size());
    if (per_address_ms < NATS_MIN_CONNECT_TIMEOUT_PER_ADDRESS_MS) {
        per_address_ms = NATS_MIN_CONNECT_TIMEOUT_PER_ADDRESS_MS;
    }

    QoreSandboxManagerHelper smh(QoreSandboxManagerHelper::Policy);
    bool denied = false;
    for (const struct addrinfo* ai : addresses) {
        // the policy is checked for each resolved address, so a host name that the server reports or that resolves
        // to another address on a reconnection is governed exactly like the first connection
        if (smh && !smh->checkNetworkAccess(host, ai->ai_addr, ai->ai_addrlen, QSEC_NET_TCP, &xsink)) {
            xsink.clear();
            denied = true;
            continue;
        }
        natsStatus s = connectAddress(fd, ai, per_address_ms, xsink);
        if (s == NATS_OK) {
            return NATS_OK;
        }
        if (xsink) {
            // the connection was cancelled or its sandbox interrupted
            keepException(xsink);
            return NATS_ERR;
        }
    }
    return denied ? NATS_NOT_PERMITTED : NATS_NO_SERVER;
}

natsStatus NatsSandboxDialer::connectAddress(natsSock* fd, const struct addrinfo* ai, int64 timeout_ms,
        ExceptionSink& xsink) {
    int sock = ::socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if (sock < 0) {
        return NATS_SYS_ERROR;
    }
    SocketHolder holder(sock);

#ifdef SO_NOSIGPIPE
    {
        int yes = 1;
        if (setsockopt(sock, SOL_SOCKET, SO_NOSIGPIPE, &yes, sizeof(yes))) {
            return NATS_SYS_ERROR;
        }
    }
#endif
    // the C client uses its sockets in non-blocking mode
    int flags = fcntl(sock, F_GETFL, 0);
    if (flags < 0 || fcntl(sock, F_SETFL, flags | O_NONBLOCK) < 0) {
        return NATS_SYS_ERROR;
    }

    if (::connect(sock, ai->ai_addr, ai->ai_addrlen)) {
        if (errno != EINPROGRESS) {
            return NATS_NO_SERVER;
        }
        struct pollfd pfd = {sock, POLLOUT, 0};
        int rc = qore_cancellable_poll(&pfd, 1, static_cast<int>(timeout_ms), &xsink, "connecting to a NATS server");
        if (rc == QORE_POLL_CANCELLED) {
            return NATS_ERR;
        }
        if (rc <= 0) {
            return rc ? NATS_SYS_ERROR : NATS_TIMEOUT;
        }
        int err = 0;
        socklen_t len = sizeof(err);
        if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &err, &len) || err) {
            return NATS_NO_SERVER;
        }
    }

    // the options that the C client sets on the sockets that it connects itself
    int yes = 1;
    struct linger l;
    l.l_onoff = 1;
    l.l_linger = 0;
    if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(yes))
        || setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes))
        || setsockopt(sock, SOL_SOCKET, SO_LINGER, &l, sizeof(l))) {
        return NATS_SYS_ERROR;
    }

    *fd = holder.release();
    return NATS_OK;
}
