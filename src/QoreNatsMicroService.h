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

#include <memory>
#include <mutex>
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

//! The endpoint handler contexts of a microservice, shared with its done handler
/** A handler context can be used by a request handler on a cnats delivery thread until the last endpoint
    subscription of the service is complete, which cnats reports with the done handler; the contexts are released
    there, never while a request can still be handled
*/
struct MicroServiceState {
    std::mutex m;
    std::vector<MicroEndpointCallbackContext*> handlers;
    //! set by the done handler: the endpoints are complete, and a context registered later is released at once
    bool done = false;
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

    //! Called by cnats when the last endpoint subscription of the service is complete
    /** Setting a done handler also makes cnats release the service when it is destroyed (cnats 3.12 releases the
        reference that the connection holds on the service only after calling the done handler)
    */
    static void serviceDone(microService* m);

    //! Releases handler contexts; can be called on a cnats thread
    static void releaseHandlers(std::vector<MicroEndpointCallbackContext*>& handlers);

private:
    microService* svc = nullptr;
    //! the handler contexts, released by the done handler
    std::shared_ptr<MicroServiceState> state;

    // non-copyable
    QoreNatsMicroService(const QoreNatsMicroService&) = delete;
    QoreNatsMicroService& operator=(const QoreNatsMicroService&) = delete;
};

#endif // _QORE_NATS_MICRO_SERVICE_H
