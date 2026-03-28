/* -*- mode: c++; indent-tabs-mode: nil -*- */
/** @file QoreNatsMicroService.cpp QoreNatsMicroService implementation */
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

#include "QoreNatsMicroService.h"
#include "QoreNatsMicroGroup.h"

microError* QoreNatsMicroService::requestHandler(microRequest* req) {
    MicroEndpointCallbackContext* ctx =
        static_cast<MicroEndpointCallbackContext*>(microRequest_GetEndpointState(req));
    if (!ctx || !ctx->handler || !ctx->pgm) {
        return micro_Errorf("no handler configured");
    }

    QoreForeignThreadHelper fth;
    ExceptionSink xsink;
    QoreExternalProgramContextHelper pch(&xsink, ctx->pgm);
    if (xsink) {
        xsink.clear();
        return micro_Errorf("failed to acquire program context");
    }

    // Build request hash
    ReferenceHolder<QoreHashNode> req_hash(new QoreHashNode(autoTypeInfo), &xsink);
    const char* subject = microRequest_GetSubject(req);
    if (subject) {
        req_hash->setKeyValue("subject", new QoreStringNode(subject), &xsink);
        if (xsink) {
            xsink.clear();
            return micro_Errorf("failed to build request hash");
        }
    }
    const char* data = microRequest_GetData(req);
    int data_len = microRequest_GetDataLength(req);
    if (data && data_len > 0) {
        BinaryNode* bin = new BinaryNode();
        bin->append(data, data_len);
        req_hash->setKeyValue("data", bin, &xsink);
        if (xsink) {
            xsink.clear();
            return micro_Errorf("failed to build request hash");
        }
    }

    // Call the Qore handler
    ReferenceHolder<QoreListNode> args(new QoreListNode(autoTypeInfo), &xsink);
    args->push(req_hash.release(), &xsink);
    if (xsink) {
        xsink.clear();
        return micro_Errorf("failed to build handler arguments");
    }
    ValueHolder rv(ctx->handler->execValue(*args, &xsink), &xsink);

    if (xsink) {
        QoreString err_msg("handler error");
        const QoreValue desc = xsink.getExceptionDesc();
        if (desc.getType() == NT_STRING) {
            err_msg.clear();
            err_msg.concat(desc.get<const QoreStringNode>()->c_str());
        }
        xsink.clear();
        return micro_Errorf("%s", err_msg.c_str());
    }

    // Send response based on return value type
    if (rv->getType() == NT_STRING) {
        const QoreStringNode* str = rv->get<const QoreStringNode>();
        return microRequest_Respond(req, str->c_str(), str->size());
    } else if (rv->getType() == NT_BINARY) {
        const BinaryNode* bin = rv->get<const BinaryNode>();
        return microRequest_Respond(req, static_cast<const char*>(bin->getPtr()),
            bin->size());
    } else if (rv->getType() == NT_NOTHING || rv->getType() == NT_NULL) {
        // Respond with empty data
        return microRequest_Respond(req, "", 0);
    }

    // Default: respond with empty data
    return microRequest_Respond(req, "", 0);
}

QoreNatsMicroService::QoreNatsMicroService(natsConnection* conn,
        const QoreHashNode* config, QoreProgram* pgm, ExceptionSink* xsink) {
    // Extract config fields
    QoreValue v_name = config->getKeyValue("name");
    QoreValue v_version = config->getKeyValue("version");
    QoreValue v_description = config->getKeyValue("description");
    QoreValue v_endpoint = config->getKeyValue("endpoint");

    if (v_name.getType() != NT_STRING || v_version.getType() != NT_STRING) {
        xsink->raiseException("NATS-MICRO-ERROR",
            "microservice config requires 'name' and 'version' fields");
        return;
    }

    // Build the microServiceConfig
    microServiceConfig svc_cfg = {};
    svc_cfg.Name = v_name.get<const QoreStringNode>()->c_str();
    svc_cfg.Version = v_version.get<const QoreStringNode>()->c_str();
    if (v_description.getType() == NT_STRING) {
        svc_cfg.Description = v_description.get<const QoreStringNode>()->c_str();
    }

    // Handle default endpoint if provided
    microEndpointConfig ep_cfg = {};
    MicroEndpointCallbackContext* ep_ctx = nullptr;

    if (v_endpoint.getType() == NT_HASH) {
        const QoreHashNode* ep_hash = v_endpoint.get<const QoreHashNode>();
        QoreValue ep_name = ep_hash->getKeyValue("name");
        QoreValue ep_subject = ep_hash->getKeyValue("subject");
        QoreValue ep_handler = ep_hash->getKeyValue("handler");

        if (ep_name.getType() != NT_STRING) {
            xsink->raiseException("NATS-MICRO-ERROR",
                "endpoint config requires 'name' field");
            return;
        }

        ep_cfg.Name = ep_name.get<const QoreStringNode>()->c_str();
        if (ep_subject.getType() == NT_STRING) {
            ep_cfg.Subject = ep_subject.get<const QoreStringNode>()->c_str();
        }

        if (ep_handler.getType() != NT_NOTHING && ep_handler.getType() != NT_NULL) {
            const ResolvedCallReferenceNode* handler =
                ep_handler.get<const ResolvedCallReferenceNode>();
            ep_ctx = new MicroEndpointCallbackContext();
            ep_ctx->handler = handler->refRefSelf();
            ep_ctx->pgm = pgm;
            ep_ctx->pgm->ref();

            ep_cfg.Handler = requestHandler;
            ep_cfg.State = ep_ctx;
        }

        svc_cfg.Endpoint = &ep_cfg;
    }

    microError* err = micro_AddService(&svc, conn, &svc_cfg);
    if (err) {
        if (ep_ctx) {
            ep_ctx->cleanup(xsink);
            delete ep_ctx;
        }
        nats_micro_error(xsink, "NATS-MICRO-ERROR", err,
            "failed to create microservice '%s'", svc_cfg.Name);
        return;
    }

    // Store handler context for cleanup
    if (ep_ctx) {
        handlers.push_back(ep_ctx);
    }
}

QoreNatsMicroService::~QoreNatsMicroService() {
    ExceptionSink xsink;

    // Clean up handler contexts
    for (auto* ctx : handlers) {
        ctx->cleanup(&xsink);
        delete ctx;
    }
    handlers.clear();

    if (svc) {
        // Stop the service first to initiate endpoint drain, then destroy.
        // microService_Destroy initiates async subscription drains that
        // free the service when complete. nats_CloseAndWait in the module
        // delete function handles waiting for pending async operations.
        microError_Ignore(microService_Stop(svc));
        microError_Ignore(microService_Destroy(svc));
        svc = nullptr;
    }
}

int QoreNatsMicroService::stop(ExceptionSink* xsink) {
    if (!svc) {
        xsink->raiseException("NATS-MICRO-ERROR", "microservice has been destroyed");
        return -1;
    }
    microError* err = microService_Stop(svc);
    if (err) {
        nats_micro_error(xsink, "NATS-MICRO-ERROR", err,
            "failed to stop microservice");
        return -1;
    }
    return 0;
}

bool QoreNatsMicroService::isStopped() const {
    if (!svc) {
        return true;
    }
    return microService_IsStopped(svc);
}

QoreHashNode* QoreNatsMicroService::getInfo(ExceptionSink* xsink) {
    if (!svc) {
        xsink->raiseException("NATS-MICRO-ERROR", "microservice has been destroyed");
        return nullptr;
    }

    microServiceInfo* info = nullptr;
    microError* err = microService_GetInfo(&info, svc);
    if (err) {
        nats_micro_error(xsink, "NATS-MICRO-ERROR", err,
            "failed to get microservice info");
        return nullptr;
    }

    ReferenceHolder<QoreHashNode> h(new QoreHashNode(hashdeclNatsMicroServiceInfo, xsink),
        xsink);
    if (*xsink) {
        microServiceInfo_Destroy(info);
        return nullptr;
    }

    if (info->Name) {
        h->setKeyValue("name", new QoreStringNode(info->Name), xsink);
        if (*xsink) {
            microServiceInfo_Destroy(info);
            return nullptr;
        }
    }
    if (info->Version) {
        h->setKeyValue("version", new QoreStringNode(info->Version), xsink);
        if (*xsink) {
            microServiceInfo_Destroy(info);
            return nullptr;
        }
    }
    if (info->Description) {
        h->setKeyValue("description", new QoreStringNode(info->Description), xsink);
        if (*xsink) {
            microServiceInfo_Destroy(info);
            return nullptr;
        }
    }
    if (info->Id) {
        h->setKeyValue("id", new QoreStringNode(info->Id), xsink);
        if (*xsink) {
            microServiceInfo_Destroy(info);
            return nullptr;
        }
    }

    // Build endpoints list
    if (info->Endpoints && info->EndpointsLen > 0) {
        ReferenceHolder<QoreListNode> ep_list(new QoreListNode(
            hashdeclNatsMicroEndpointInfo->getTypeInfo()), xsink);
        for (int i = 0; i < info->EndpointsLen; ++i) {
            ReferenceHolder<QoreHashNode> ep(
                new QoreHashNode(hashdeclNatsMicroEndpointInfo, xsink), xsink);
            if (*xsink) {
                microServiceInfo_Destroy(info);
                return nullptr;
            }
            if (info->Endpoints[i].Name) {
                ep->setKeyValue("name", new QoreStringNode(info->Endpoints[i].Name), xsink);
                if (*xsink) {
                    microServiceInfo_Destroy(info);
                    return nullptr;
                }
            }
            if (info->Endpoints[i].Subject) {
                ep->setKeyValue("subject", new QoreStringNode(info->Endpoints[i].Subject),
                    xsink);
                if (*xsink) {
                    microServiceInfo_Destroy(info);
                    return nullptr;
                }
            }
            ep_list->push(ep.release(), xsink);
            if (*xsink) {
                microServiceInfo_Destroy(info);
                return nullptr;
            }
        }
        h->setKeyValue("endpoints", ep_list.release(), xsink);
        if (*xsink) {
            microServiceInfo_Destroy(info);
            return nullptr;
        }
    }

    microServiceInfo_Destroy(info);
    return h.release();
}

QoreHashNode* QoreNatsMicroService::getStats(ExceptionSink* xsink) {
    if (!svc) {
        xsink->raiseException("NATS-MICRO-ERROR", "microservice has been destroyed");
        return nullptr;
    }

    microServiceStats* stats = nullptr;
    microError* err = microService_GetStats(&stats, svc);
    if (err) {
        nats_micro_error(xsink, "NATS-MICRO-ERROR", err,
            "failed to get microservice stats");
        return nullptr;
    }

    ReferenceHolder<QoreHashNode> h(new QoreHashNode(hashdeclNatsMicroServiceStats, xsink),
        xsink);
    if (*xsink) {
        microServiceStats_Destroy(stats);
        return nullptr;
    }

    if (stats->Name) {
        h->setKeyValue("name", new QoreStringNode(stats->Name), xsink);
        if (*xsink) {
            microServiceStats_Destroy(stats);
            return nullptr;
        }
    }
    if (stats->Version) {
        h->setKeyValue("version", new QoreStringNode(stats->Version), xsink);
        if (*xsink) {
            microServiceStats_Destroy(stats);
            return nullptr;
        }
    }
    if (stats->Id) {
        h->setKeyValue("id", new QoreStringNode(stats->Id), xsink);
        if (*xsink) {
            microServiceStats_Destroy(stats);
            return nullptr;
        }
    }

    // Started timestamp - int64_t nanoseconds since epoch
    if (stats->Started > 0) {
        int64 started_us = stats->Started / 1000;
        h->setKeyValue("started", DateTimeNode::makeAbsolute(
            currentTZ(), started_us / 1000000, (int)(started_us % 1000000)), xsink);
        if (*xsink) {
            microServiceStats_Destroy(stats);
            return nullptr;
        }
    }

    // Build endpoints list
    if (stats->Endpoints && stats->EndpointsLen > 0) {
        ReferenceHolder<QoreListNode> ep_list(new QoreListNode(
            hashdeclNatsMicroEndpointStats->getTypeInfo()), xsink);
        for (int i = 0; i < stats->EndpointsLen; ++i) {
            ReferenceHolder<QoreHashNode> ep(
                new QoreHashNode(hashdeclNatsMicroEndpointStats, xsink), xsink);
            if (*xsink) {
                microServiceStats_Destroy(stats);
                return nullptr;
            }
            if (stats->Endpoints[i].Name) {
                ep->setKeyValue("name", new QoreStringNode(stats->Endpoints[i].Name), xsink);
                if (*xsink) {
                    microServiceStats_Destroy(stats);
                    return nullptr;
                }
            }
            if (stats->Endpoints[i].Subject) {
                ep->setKeyValue("subject",
                    new QoreStringNode(stats->Endpoints[i].Subject), xsink);
                if (*xsink) {
                    microServiceStats_Destroy(stats);
                    return nullptr;
                }
            }
            ep->setKeyValue("num_requests",
                (int64)stats->Endpoints[i].NumRequests, xsink);
            if (*xsink) {
                microServiceStats_Destroy(stats);
                return nullptr;
            }
            ep->setKeyValue("num_errors",
                (int64)stats->Endpoints[i].NumErrors, xsink);
            if (*xsink) {
                microServiceStats_Destroy(stats);
                return nullptr;
            }
            // Combine seconds and nanoseconds into total nanoseconds
            int64 total_ns = stats->Endpoints[i].ProcessingTimeSeconds * 1000000000LL
                + stats->Endpoints[i].ProcessingTimeNanoseconds;
            ep->setKeyValue("processing_time_ns", total_ns, xsink);
            if (*xsink) {
                microServiceStats_Destroy(stats);
                return nullptr;
            }
            ep->setKeyValue("average_processing_time_ns",
                (int64)stats->Endpoints[i].AverageProcessingTimeNanoseconds, xsink);
            if (*xsink) {
                microServiceStats_Destroy(stats);
                return nullptr;
            }
            if (stats->Endpoints[i].LastErrorString[0] != '\0') {
                ep->setKeyValue("last_error",
                    new QoreStringNode(stats->Endpoints[i].LastErrorString), xsink);
                if (*xsink) {
                    microServiceStats_Destroy(stats);
                    return nullptr;
                }
            }
            ep_list->push(ep.release(), xsink);
            if (*xsink) {
                microServiceStats_Destroy(stats);
                return nullptr;
            }
        }
        h->setKeyValue("endpoints", ep_list.release(), xsink);
        if (*xsink) {
            microServiceStats_Destroy(stats);
            return nullptr;
        }
    }

    microServiceStats_Destroy(stats);
    return h.release();
}

QoreNatsMicroGroup* QoreNatsMicroService::addGroup(const char* prefix,
        ExceptionSink* xsink) {
    if (!svc) {
        xsink->raiseException("NATS-MICRO-ERROR", "microservice has been destroyed");
        return nullptr;
    }

    microGroupConfig grp_cfg = {};
    grp_cfg.Prefix = prefix;

    microGroup* grp = nullptr;
    microError* err = microService_AddGroup(&grp, svc, &grp_cfg);
    if (err) {
        nats_micro_error(xsink, "NATS-MICRO-ERROR", err,
            "failed to add group '%s'", prefix);
        return nullptr;
    }

    return new QoreNatsMicroGroup(grp, this);
}

int QoreNatsMicroService::addEndpoint(const QoreHashNode* config,
        QoreProgram* pgm, ExceptionSink* xsink) {
    if (!svc) {
        xsink->raiseException("NATS-MICRO-ERROR", "microservice has been destroyed");
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

        ep_cfg.Handler = requestHandler;
        ep_cfg.State = ctx;
    }

    microError* err = microService_AddEndpoint(svc, &ep_cfg);
    if (err) {
        if (ctx) {
            ctx->cleanup(xsink);
            delete ctx;
        }
        nats_micro_error(xsink, "NATS-MICRO-ERROR", err,
            "failed to add endpoint '%s'", ep_cfg.Name);
        return -1;
    }

    if (ctx) {
        handlers.push_back(ctx);
    }
    return 0;
}

void QoreNatsMicroService::registerHandler(MicroEndpointCallbackContext* ctx) {
    handlers.push_back(ctx);
}
