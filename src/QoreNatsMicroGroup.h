/* -*- mode: c++; indent-tabs-mode: nil -*- */
/** @file QoreNatsMicroGroup.h QoreNatsMicroGroup class definition */
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

#ifndef _QORE_NATS_MICRO_GROUP_H
#define _QORE_NATS_MICRO_GROUP_H

#include "nats-module.h"
#include "NatsHelper.h"
#include "QoreNatsMicroService.h"

//! C++ wrapper for microGroup
/** The microGroup is owned by the microService; we do NOT destroy it.
    We keep a reference to the owning QoreNatsMicroService for handler registration.
*/
class QoreNatsMicroGroup : public AbstractPrivateData {
public:
    //! Constructor
    DLLLOCAL QoreNatsMicroGroup(microGroup* g, QoreNatsMicroService* svc);

    //! Destructor
    DLLLOCAL virtual ~QoreNatsMicroGroup();

    //! Add a sub-group
    DLLLOCAL QoreNatsMicroGroup* addGroup(const char* prefix, ExceptionSink* xsink);

    //! Add an endpoint to the group
    DLLLOCAL int addEndpoint(const QoreHashNode* config, QoreProgram* pgm,
        ExceptionSink* xsink);

private:
    microGroup* group = nullptr;
    QoreNatsMicroService* svc;  //!< borrowed reference to owning service

    // non-copyable
    QoreNatsMicroGroup(const QoreNatsMicroGroup&) = delete;
    QoreNatsMicroGroup& operator=(const QoreNatsMicroGroup&) = delete;
};

#endif // _QORE_NATS_MICRO_GROUP_H
