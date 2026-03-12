/* -*- mode: c++; indent-tabs-mode: nil -*- */
/** @file QoreNatsKVStore.h QoreNatsKVStore class definition */
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

#ifndef _QORE_NATS_KVSTORE_H
#define _QORE_NATS_KVSTORE_H

#include "nats-module.h"
#include "NatsHelper.h"

//! C++ wrapper for kvStore (NATS Key-Value store)
class QoreNatsKVStore : public AbstractPrivateData {
public:
    //! Constructor - takes ownership of the kvStore
    DLLLOCAL QoreNatsKVStore(kvStore* kv);

    //! Destructor
    DLLLOCAL virtual ~QoreNatsKVStore();

    //! Put a value (returns revision)
    DLLLOCAL int64 put(const char* key, const void* data, int data_len,
        ExceptionSink* xsink);

    //! Put a string value (returns revision)
    DLLLOCAL int64 putString(const char* key, const char* data,
        ExceptionSink* xsink);

    //! Create a value (only if not exists, returns revision)
    DLLLOCAL int64 create(const char* key, const void* data, int data_len,
        ExceptionSink* xsink);

    //! Update a value (CAS with revision, returns new revision)
    DLLLOCAL int64 update(const char* key, const void* data, int data_len,
        uint64_t revision, ExceptionSink* xsink);

    //! Get a value
    DLLLOCAL QoreHashNode* get(const char* key, ExceptionSink* xsink);

    //! Delete a key
    DLLLOCAL int del(const char* key, ExceptionSink* xsink);

    //! Purge a key (remove all revisions)
    DLLLOCAL int purge(const char* key, ExceptionSink* xsink);

    //! List all keys
    DLLLOCAL QoreListNode* keys(ExceptionSink* xsink);

    //! Get history for a key
    DLLLOCAL QoreListNode* history(const char* key, ExceptionSink* xsink);

    //! Get bucket name
    DLLLOCAL const char* bucketName() const;

private:
    kvStore* kv = nullptr;

    //! Convert a kvEntry to a hash
    DLLLOCAL QoreHashNode* entryToHash(kvEntry* entry, ExceptionSink* xsink);

    // non-copyable
    QoreNatsKVStore(const QoreNatsKVStore&) = delete;
    QoreNatsKVStore& operator=(const QoreNatsKVStore&) = delete;
};

#endif // _QORE_NATS_KVSTORE_H
