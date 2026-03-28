/* -*- mode: c++; indent-tabs-mode: nil -*- */
/** @file QoreNatsMicroService.h QoreNatsMicroService class definition */
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

#ifndef _QORE_NATS_MICRO_SERVICE_H
#define _QORE_NATS_MICRO_SERVICE_H

#include "nats-module.h"
#include "NatsHelper.h"

#include <vector>

class QoreNatsMicroGroup;

//! Callback context for bridging nats.c micro request handlers to Qore closures
struct MicroEndpointCallbackContext {
    ResolvedCallReferenceNode* handler = nullptr;
    QoreProgram* pgm = nullptr;

    DLLLOCAL void cleanup(ExceptionSink* xsink) {
        if (handler) {
            handler->deref(xsink);
            handler = nullptr;
        }
        if (pgm) {
            pgm->deref(xsink);
            pgm = nullptr;
        }
    }
};

//! C++ wrapper for microService
class QoreNatsMicroService : public AbstractPrivateData {
public:
    //! Constructor
    DLLLOCAL QoreNatsMicroService(natsConnection* conn, const QoreHashNode* config,
        QoreProgram* pgm, ExceptionSink* xsink);

    //! Destructor
    DLLLOCAL virtual ~QoreNatsMicroService();

    //! Stop the microservice
    DLLLOCAL int stop(ExceptionSink* xsink);

    //! Check if the microservice is stopped
    DLLLOCAL bool isStopped() const;

    //! Get service information
    DLLLOCAL QoreHashNode* getInfo(ExceptionSink* xsink);

    //! Get service statistics
    DLLLOCAL QoreHashNode* getStats(ExceptionSink* xsink);

    //! Add a group to the service
    DLLLOCAL QoreNatsMicroGroup* addGroup(const char* prefix, ExceptionSink* xsink);

    //! Add an endpoint to the service
    DLLLOCAL int addEndpoint(const QoreHashNode* config, QoreProgram* pgm,
        ExceptionSink* xsink);

    //! Register a handler context for cleanup
    DLLLOCAL void registerHandler(MicroEndpointCallbackContext* ctx);

    //! Static request handler that bridges nats.c micro to Qore closures
    static microError* requestHandler(microRequest* req);

private:
    microService* svc = nullptr;
    std::vector<MicroEndpointCallbackContext*> handlers;

    // non-copyable
    QoreNatsMicroService(const QoreNatsMicroService&) = delete;
    QoreNatsMicroService& operator=(const QoreNatsMicroService&) = delete;
};

#endif // _QORE_NATS_MICRO_SERVICE_H
