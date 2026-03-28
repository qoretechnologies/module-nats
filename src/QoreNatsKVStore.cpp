/* -*- mode: c++; indent-tabs-mode: nil -*- */
/** @file QoreNatsKVStore.cpp QoreNatsKVStore implementation */
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

#include "QoreNatsKVStore.h"

QoreNatsKVStore::QoreNatsKVStore(kvStore* kv) : kv(kv) {
}

QoreNatsKVStore::~QoreNatsKVStore() {
    if (kv) {
        kvStore_Destroy(kv);
        kv = nullptr;
    }
}

QoreHashNode* QoreNatsKVStore::entryToHash(kvEntry* entry, ExceptionSink* xsink) {
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

int64 QoreNatsKVStore::put(const char* key, const void* data, int data_len,
        ExceptionSink* xsink) {
    if (!kv) {
        xsink->raiseException("NATS-KV-ERROR", "KV store is not valid");
        return -1;
    }
    if (qore_check_cancel(xsink)) {
        return -1;
    }

    uint64_t rev = 0;
    natsStatus s = kvStore_Put(&rev, kv, key, data, data_len);
    if (s != NATS_OK) {
        nats_error(xsink, "NATS-KV-ERROR", s,
            "failed to put key '%s'", key);
        return -1;
    }
    return (int64)rev;
}

int64 QoreNatsKVStore::putString(const char* key, const char* data,
        ExceptionSink* xsink) {
    if (!kv) {
        xsink->raiseException("NATS-KV-ERROR", "KV store is not valid");
        return -1;
    }
    if (qore_check_cancel(xsink)) {
        return -1;
    }

    uint64_t rev = 0;
    natsStatus s = kvStore_PutString(&rev, kv, key, data);
    if (s != NATS_OK) {
        nats_error(xsink, "NATS-KV-ERROR", s,
            "failed to put string key '%s'", key);
        return -1;
    }
    return (int64)rev;
}

int64 QoreNatsKVStore::create(const char* key, const void* data, int data_len,
        ExceptionSink* xsink) {
    if (!kv) {
        xsink->raiseException("NATS-KV-ERROR", "KV store is not valid");
        return -1;
    }
    if (qore_check_cancel(xsink)) {
        return -1;
    }

    uint64_t rev = 0;
    natsStatus s = kvStore_Create(&rev, kv, key, data, data_len);
    if (s != NATS_OK) {
        nats_error(xsink, "NATS-KV-ERROR", s,
            "failed to create key '%s'", key);
        return -1;
    }
    return (int64)rev;
}

int64 QoreNatsKVStore::update(const char* key, const void* data, int data_len,
        uint64_t revision, ExceptionSink* xsink) {
    if (!kv) {
        xsink->raiseException("NATS-KV-ERROR", "KV store is not valid");
        return -1;
    }
    if (qore_check_cancel(xsink)) {
        return -1;
    }

    uint64_t rev = 0;
    natsStatus s = kvStore_Update(&rev, kv, key, data, data_len, revision);
    if (s != NATS_OK) {
        nats_error(xsink, "NATS-KV-ERROR", s,
            "failed to update key '%s' at revision %llu", key,
            (unsigned long long)revision);
        return -1;
    }
    return (int64)rev;
}

QoreHashNode* QoreNatsKVStore::get(const char* key, ExceptionSink* xsink) {
    if (!kv) {
        xsink->raiseException("NATS-KV-ERROR", "KV store is not valid");
        return nullptr;
    }
    if (qore_check_cancel(xsink)) {
        return nullptr;
    }

    kvEntry* entry = nullptr;
    natsStatus s = kvStore_Get(&entry, kv, key);
    if (s == NATS_NOT_FOUND) {
        return nullptr;  // not an error, just not found
    }
    if (s != NATS_OK) {
        nats_error(xsink, "NATS-KV-ERROR", s,
            "failed to get key '%s'", key);
        return nullptr;
    }

    QoreHashNode* rv = entryToHash(entry, xsink);
    kvEntry_Destroy(entry);
    return rv;
}

int QoreNatsKVStore::del(const char* key, ExceptionSink* xsink) {
    if (!kv) {
        xsink->raiseException("NATS-KV-ERROR", "KV store is not valid");
        return -1;
    }
    if (qore_check_cancel(xsink)) {
        return -1;
    }

    natsStatus s = kvStore_Delete(kv, key);
    if (s != NATS_OK) {
        nats_error(xsink, "NATS-KV-ERROR", s,
            "failed to delete key '%s'", key);
        return -1;
    }
    return 0;
}

int QoreNatsKVStore::purge(const char* key, ExceptionSink* xsink) {
    if (!kv) {
        xsink->raiseException("NATS-KV-ERROR", "KV store is not valid");
        return -1;
    }
    if (qore_check_cancel(xsink)) {
        return -1;
    }

    natsStatus s = kvStore_Purge(kv, key, nullptr);
    if (s != NATS_OK) {
        nats_error(xsink, "NATS-KV-ERROR", s,
            "failed to purge key '%s'", key);
        return -1;
    }
    return 0;
}

QoreListNode* QoreNatsKVStore::keys(ExceptionSink* xsink) {
    if (!kv) {
        xsink->raiseException("NATS-KV-ERROR", "KV store is not valid");
        return nullptr;
    }
    if (qore_check_cancel(xsink)) {
        return nullptr;
    }

    kvKeysList kl;
    natsStatus s = kvStore_Keys(&kl, kv, nullptr);
    if (s == NATS_NOT_FOUND) {
        return new QoreListNode(stringTypeInfo);
    }
    if (s != NATS_OK) {
        nats_error(xsink, "NATS-KV-ERROR", s, "failed to list keys");
        return nullptr;
    }

    ReferenceHolder<QoreListNode> list(new QoreListNode(stringTypeInfo), xsink);
    for (int i = 0; i < kl.Count; ++i) {
        list->push(new QoreStringNode(kl.Keys[i]), xsink);
        if (*xsink) {
            kvKeysList_Destroy(&kl);
            return nullptr;
        }
    }
    kvKeysList_Destroy(&kl);

    return list.release();
}

QoreListNode* QoreNatsKVStore::history(const char* key, ExceptionSink* xsink) {
    if (!kv) {
        xsink->raiseException("NATS-KV-ERROR", "KV store is not valid");
        return nullptr;
    }
    if (qore_check_cancel(xsink)) {
        return nullptr;
    }

    kvEntryList el;
    natsStatus s = kvStore_History(&el, kv, key, nullptr);
    if (s == NATS_NOT_FOUND) {
        return new QoreListNode(autoHashTypeInfo);
    }
    if (s != NATS_OK) {
        nats_error(xsink, "NATS-KV-ERROR", s,
            "failed to get history for key '%s'", key);
        return nullptr;
    }

    ReferenceHolder<QoreListNode> list(new QoreListNode(autoHashTypeInfo), xsink);
    for (int i = 0; i < el.Count; ++i) {
        QoreHashNode* entry_hash = entryToHash(el.Entries[i], xsink);
        if (*xsink) {
            kvEntryList_Destroy(&el);
            return nullptr;
        }
        list->push(entry_hash, xsink);
    }
    kvEntryList_Destroy(&el);

    return list.release();
}

const char* QoreNatsKVStore::bucketName() const {
    if (!kv) {
        return "";
    }
    return kvStore_Bucket(kv);
}

QoreHashNode* QoreNatsKVStore::status(ExceptionSink* xsink) {
    if (!kv) {
        xsink->raiseException("NATS-KV-ERROR", "KV store is not valid");
        return nullptr;
    }
    if (qore_check_cancel(xsink)) {
        return nullptr;
    }

    kvStatus* sts = nullptr;
    natsStatus s = kvStore_Status(&sts, kv);
    if (s != NATS_OK) {
        nats_error(xsink, "NATS-KV-ERROR", s, "failed to get bucket status");
        return nullptr;
    }

    ReferenceHolder<QoreHashNode> h(new QoreHashNode(hashdeclNatsKVBucketStatus, xsink), xsink);
    if (*xsink) {
        kvStatus_Destroy(sts);
        return nullptr;
    }

    const char* bucket = kvStatus_Bucket(sts);
    if (bucket) {
        h->setKeyValue("bucket", new QoreStringNode(bucket), xsink);
        if (*xsink) {
            kvStatus_Destroy(sts);
            return nullptr;
        }
    }
    if (!*xsink) {
        h->setKeyValue("values", (int64)kvStatus_Values(sts), xsink);
    }
    if (!*xsink) {
        h->setKeyValue("history", (int64)kvStatus_History(sts), xsink);
    }
    if (!*xsink) {
        h->setKeyValue("ttl_ms", (int64)(kvStatus_TTL(sts) / 1000000LL), xsink);
    }
    if (!*xsink) {
        h->setKeyValue("replicas", (int64)kvStatus_Replicas(sts), xsink);
    }
    if (!*xsink) {
        h->setKeyValue("bytes", (int64)kvStatus_Bytes(sts), xsink);
    }
    kvStatus_Destroy(sts);

    if (*xsink) {
        return nullptr;
    }
    return h.release();
}
