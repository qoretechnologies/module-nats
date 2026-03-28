/* -*- mode: c++; indent-tabs-mode: nil -*- */
/** @file QoreNatsMicroGroup.cpp QoreNatsMicroGroup implementation */
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

#include "QoreNatsMicroGroup.h"

QoreNatsMicroGroup::QoreNatsMicroGroup(microGroup* g, QoreNatsMicroService* svc)
    : group(g), svc(svc) {
}

QoreNatsMicroGroup::~QoreNatsMicroGroup() {
    // microGroup is owned by the microService; we do NOT destroy it
}

QoreNatsMicroGroup* QoreNatsMicroGroup::addGroup(const char* prefix,
        ExceptionSink* xsink) {
    if (!group) {
        xsink->raiseException("NATS-MICRO-ERROR", "group has been destroyed");
        return nullptr;
    }

    microGroupConfig grp_cfg = {};
    grp_cfg.Prefix = prefix;

    microGroup* sub_grp = nullptr;
    microError* err = microGroup_AddGroup(&sub_grp, group, &grp_cfg);
    if (err) {
        nats_micro_error(xsink, "NATS-MICRO-ERROR", err,
            "failed to add sub-group '%s'", prefix);
        return nullptr;
    }

    return new QoreNatsMicroGroup(sub_grp, svc);
}

int QoreNatsMicroGroup::addEndpoint(const QoreHashNode* config,
        QoreProgram* pgm, ExceptionSink* xsink) {
    if (!group) {
        xsink->raiseException("NATS-MICRO-ERROR", "group has been destroyed");
        return -1;
    }

    QoreValue ep_name = config->getKeyValue("name");
    QoreValue ep_subject = config->getKeyValue("subject");
    QoreValue ep_handler = config->getKeyValue("handler");

    if (ep_name.getType() != NT_STRING) {
        xsink->raiseException("NATS-MICRO-ERROR",
            "endpoint config requires 'name' field");
        return -1;
    }

    microEndpointConfig ep_cfg = {};
    ep_cfg.Name = ep_name.get<const QoreStringNode>()->c_str();
    if (ep_subject.getType() == NT_STRING) {
        ep_cfg.Subject = ep_subject.get<const QoreStringNode>()->c_str();
    }

    MicroEndpointCallbackContext* ctx = nullptr;
    if (ep_handler.getType() != NT_NOTHING && ep_handler.getType() != NT_NULL) {
        const ResolvedCallReferenceNode* handler =
            ep_handler.get<const ResolvedCallReferenceNode>();
        ctx = new MicroEndpointCallbackContext();
        ctx->handler = handler->refRefSelf();
        ctx->pgm = pgm;
        ctx->pgm->ref();

        ep_cfg.Handler = QoreNatsMicroService::requestHandler;
        ep_cfg.State = ctx;
    }

    microError* err = microGroup_AddEndpoint(group, &ep_cfg);
    if (err) {
        if (ctx) {
            ctx->cleanup(xsink);
            delete ctx;
        }
        nats_micro_error(xsink, "NATS-MICRO-ERROR", err,
            "failed to add endpoint '%s' to group", ep_cfg.Name);
        return -1;
    }

    // Register with the owning service for cleanup
    if (ctx) {
        svc->registerHandler(ctx);
    }
    return 0;
}
