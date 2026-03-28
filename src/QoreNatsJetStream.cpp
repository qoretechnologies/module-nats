/* -*- mode: c++; indent-tabs-mode: nil -*- */
/** @file QoreNatsJetStream.cpp QoreNatsJetStream implementation */
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

#include "QoreNatsJetStream.h"
#include "QoreNatsSubscription.h"
#include "QoreNatsKVStore.h"

#include <cstring>

QoreNatsJetStream::QoreNatsJetStream(natsConnection* conn, ExceptionSink* xsink)
        : conn(conn) {
    jsOptions jo;
    jsOptions_Init(&jo);
    natsStatus s = natsConnection_JetStream(&js, conn, &jo);
    if (s != NATS_OK) {
        nats_error(xsink, "NATS-JETSTREAM-ERROR", s,
            "failed to create JetStream context");
    }
}

QoreNatsJetStream::~QoreNatsJetStream() {
    if (js) {
        jsCtx_Destroy(js);
        js = nullptr;
    }
}

int QoreNatsJetStream::configureStreamConfig(jsStreamConfig* cfg,
        const QoreHashNode* config, ExceptionSink* xsink) {
    jsStreamConfig_Init(cfg);

    QoreValue v = config->getKeyValue("name");
    if (v.getType() == NT_STRING) {
        cfg->Name = v.get<const QoreStringNode>()->c_str();
    }

    v = config->getKeyValue("description");
    if (v.getType() == NT_STRING) {
        cfg->Description = v.get<const QoreStringNode>()->c_str();
    }

    v = config->getKeyValue("subjects");
    if (v.getType() == NT_LIST) {
        const QoreListNode* subjects = v.get<const QoreListNode>();
        cfg->SubjectsLen = (int)subjects->size();
        // Allocate array of const char*
        const char** subj_arr = (const char**)malloc(sizeof(const char*) * cfg->SubjectsLen);
        if (!subj_arr) {
            xsink->raiseException("NATS-JETSTREAM-ERROR", "memory allocation failed for %d subjects",
                cfg->SubjectsLen);
            return -1;
        }
        for (int i = 0; i < cfg->SubjectsLen; ++i) {
            QoreValue sv = subjects->retrieveEntry(i);
            if (sv.getType() == NT_STRING) {
                subj_arr[i] = sv.get<const QoreStringNode>()->c_str();
            } else {
                subj_arr[i] = "";
            }
        }
        cfg->Subjects = subj_arr;
    }

    v = config->getKeyValue("retention");
    if (v.getType() == NT_INT) {
        cfg->Retention = (jsRetentionPolicy)v.getAsBigInt();
    }

    v = config->getKeyValue("max_msgs");
    if (v.getType() == NT_INT) {
        cfg->MaxMsgs = v.getAsBigInt();
    }

    v = config->getKeyValue("max_bytes");
    if (v.getType() == NT_INT) {
        cfg->MaxBytes = v.getAsBigInt();
    }

    v = config->getKeyValue("max_age");
    if (v.getType() == NT_INT) {
        cfg->MaxAge = v.getAsBigInt() * 1000000LL;  // ms to ns
    }

    v = config->getKeyValue("max_msg_size");
    if (v.getType() == NT_INT) {
        cfg->MaxMsgSize = (int32_t)v.getAsBigInt();
    }

    v = config->getKeyValue("storage");
    if (v.getType() == NT_INT) {
        cfg->Storage = (jsStorageType)v.getAsBigInt();
    }

    v = config->getKeyValue("num_replicas");
    if (v.getType() == NT_INT) {
        cfg->Replicas = (int)v.getAsBigInt();
    }

    v = config->getKeyValue("discard");
    if (v.getType() == NT_INT) {
        cfg->Discard = (jsDiscardPolicy)v.getAsBigInt();
    }

    v = config->getKeyValue("max_msgs_per_subject");
    if (v.getType() == NT_INT) {
        cfg->MaxMsgsPerSubject = v.getAsBigInt();
    }

    v = config->getKeyValue("max_consumers");
    if (v.getType() == NT_INT) {
        cfg->MaxConsumers = v.getAsBigInt();
    }

    v = config->getKeyValue("duplicate_window_ms");
    if (v.getType() == NT_INT) {
        cfg->Duplicates = v.getAsBigInt() * 1000000LL;  // ms to ns
    }

    v = config->getKeyValue("sealed");
    if (v.getType() == NT_BOOLEAN) {
        cfg->Sealed = v.getAsBool();
    }

    v = config->getKeyValue("deny_delete");
    if (v.getType() == NT_BOOLEAN) {
        cfg->DenyDelete = v.getAsBool();
    }

    v = config->getKeyValue("deny_purge");
    if (v.getType() == NT_BOOLEAN) {
        cfg->DenyPurge = v.getAsBool();
    }

    v = config->getKeyValue("allow_rollup");
    if (v.getType() == NT_BOOLEAN) {
        cfg->AllowRollup = v.getAsBool();
    }

    v = config->getKeyValue("no_ack");
    if (v.getType() == NT_BOOLEAN) {
        cfg->NoAck = v.getAsBool();
    }

    v = config->getKeyValue("allow_direct");
    if (v.getType() == NT_BOOLEAN) {
        cfg->AllowDirect = v.getAsBool();
    }

    v = config->getKeyValue("mirror_direct");
    if (v.getType() == NT_BOOLEAN) {
        cfg->MirrorDirect = v.getAsBool();
    }

    v = config->getKeyValue("discard_new_per_subject");
    if (v.getType() == NT_BOOLEAN) {
        cfg->DiscardNewPerSubject = v.getAsBool();
    }

    return 0;
}

QoreHashNode* QoreNatsJetStream::streamInfoToHash(jsStreamInfo* info,
        ExceptionSink* xsink) {
    ReferenceHolder<QoreHashNode> h(new QoreHashNode(hashdeclNatsStreamInfo, xsink), xsink);
    if (*xsink) {
        return nullptr;
    }

    // Config sub-hash
    ReferenceHolder<QoreHashNode> cfg(new QoreHashNode(hashdeclNatsStreamConfig, xsink), xsink);
    if (*xsink) {
        return nullptr;
    }
    if (info->Config) {
        if (info->Config->Name) {
            cfg->setKeyValue("name", new QoreStringNode(info->Config->Name), xsink);
        }
        if (!*xsink && info->Config->Description) {
            cfg->setKeyValue("description", new QoreStringNode(info->Config->Description), xsink);
        }
        if (!*xsink) {
            cfg->setKeyValue("retention", (int64)info->Config->Retention, xsink);
        }
        if (!*xsink) {
            cfg->setKeyValue("max_msgs", (int64)info->Config->MaxMsgs, xsink);
        }
        if (!*xsink) {
            cfg->setKeyValue("max_bytes", (int64)info->Config->MaxBytes, xsink);
        }
        if (!*xsink) {
            cfg->setKeyValue("max_age", (int64)(info->Config->MaxAge / 1000000LL), xsink);
        }
        if (!*xsink) {
            cfg->setKeyValue("max_msg_size", (int64)info->Config->MaxMsgSize, xsink);
        }
        if (!*xsink) {
            cfg->setKeyValue("storage", (int64)info->Config->Storage, xsink);
        }
        if (!*xsink) {
            cfg->setKeyValue("num_replicas", (int64)info->Config->Replicas, xsink);
        }
        if (!*xsink) {
            cfg->setKeyValue("discard", (int64)info->Config->Discard, xsink);
        }
        if (!*xsink) {
            cfg->setKeyValue("max_msgs_per_subject", (int64)info->Config->MaxMsgsPerSubject, xsink);
        }
        if (!*xsink) {
            cfg->setKeyValue("max_consumers", (int64)info->Config->MaxConsumers, xsink);
        }
        if (!*xsink) {
            cfg->setKeyValue("duplicate_window_ms",
                (int64)(info->Config->Duplicates / 1000000LL), xsink);
        }
        if (!*xsink) {
            cfg->setKeyValue("sealed", info->Config->Sealed, xsink);
        }
        if (!*xsink) {
            cfg->setKeyValue("deny_delete", info->Config->DenyDelete, xsink);
        }
        if (!*xsink) {
            cfg->setKeyValue("deny_purge", info->Config->DenyPurge, xsink);
        }
        if (!*xsink) {
            cfg->setKeyValue("allow_rollup", info->Config->AllowRollup, xsink);
        }
        if (!*xsink) {
            cfg->setKeyValue("no_ack", info->Config->NoAck, xsink);
        }
        if (!*xsink) {
            cfg->setKeyValue("allow_direct", info->Config->AllowDirect, xsink);
        }
        if (!*xsink) {
            cfg->setKeyValue("mirror_direct", info->Config->MirrorDirect, xsink);
        }
        if (!*xsink) {
            cfg->setKeyValue("discard_new_per_subject",
                info->Config->DiscardNewPerSubject, xsink);
        }
        if (*xsink) {
            return nullptr;
        }

        // Subjects
        if (info->Config->SubjectsLen > 0 && info->Config->Subjects) {
            ReferenceHolder<QoreListNode> subjects(new QoreListNode(stringTypeInfo), xsink);
            for (int i = 0; i < info->Config->SubjectsLen; ++i) {
                subjects->push(new QoreStringNode(info->Config->Subjects[i]), xsink);
                if (*xsink) {
                    return nullptr;
                }
            }
            cfg->setKeyValue("subjects", subjects.release(), xsink);
        }
    }
    if (*xsink) {
        return nullptr;
    }
    h->setKeyValue("config", cfg.release(), xsink);

    if (!*xsink) {
        h->setKeyValue("messages", (int64)info->State.Msgs, xsink);
    }
    if (!*xsink) {
        h->setKeyValue("bytes", (int64)info->State.Bytes, xsink);
    }
    if (!*xsink) {
        h->setKeyValue("first_seq", (int64)info->State.FirstSeq, xsink);
    }
    if (!*xsink) {
        h->setKeyValue("last_seq", (int64)info->State.LastSeq, xsink);
    }
    if (!*xsink) {
        h->setKeyValue("consumer_count", (int64)info->State.Consumers, xsink);
    }
    if (*xsink) {
        return nullptr;
    }

    return h.release();
}

QoreHashNode* QoreNatsJetStream::addStream(const QoreHashNode* config,
        ExceptionSink* xsink) {
    if (!js) {
        xsink->raiseException("NATS-JETSTREAM-ERROR", "JetStream context is not valid");
        return nullptr;
    }
    if (qore_check_cancel(xsink)) {
        return nullptr;
    }

    jsStreamConfig cfg;
    configureStreamConfig(&cfg, config, xsink);
    if (*xsink) {
        if (cfg.Subjects) {
            free((void*)cfg.Subjects);
        }
        return nullptr;
    }

    jsStreamInfo* info = nullptr;
    jsErrCode jerr{};
    natsStatus s = js_AddStream(&info, js, &cfg, nullptr, &jerr);

    if (cfg.Subjects) {
        free((void*)cfg.Subjects);
    }

    if (s != NATS_OK) {
        nats_js_error(xsink, "NATS-JETSTREAM-ERROR", s, jerr, "failed to add stream");
        return nullptr;
    }

    QoreHashNode* rv = streamInfoToHash(info, xsink);
    jsStreamInfo_Destroy(info);
    return rv;
}

QoreHashNode* QoreNatsJetStream::updateStream(const QoreHashNode* config,
        ExceptionSink* xsink) {
    if (!js) {
        xsink->raiseException("NATS-JETSTREAM-ERROR", "JetStream context is not valid");
        return nullptr;
    }
    if (qore_check_cancel(xsink)) {
        return nullptr;
    }

    jsStreamConfig cfg;
    configureStreamConfig(&cfg, config, xsink);
    if (*xsink) {
        if (cfg.Subjects) {
            free((void*)cfg.Subjects);
        }
        return nullptr;
    }

    jsStreamInfo* info = nullptr;
    jsErrCode jerr{};
    natsStatus s = js_UpdateStream(&info, js, &cfg, nullptr, &jerr);

    if (cfg.Subjects) {
        free((void*)cfg.Subjects);
    }

    if (s != NATS_OK) {
        nats_js_error(xsink, "NATS-JETSTREAM-ERROR", s, jerr, "failed to update stream");
        return nullptr;
    }

    QoreHashNode* rv = streamInfoToHash(info, xsink);
    jsStreamInfo_Destroy(info);
    return rv;
}

int QoreNatsJetStream::deleteStream(const char* name, ExceptionSink* xsink) {
    if (!js) {
        xsink->raiseException("NATS-JETSTREAM-ERROR", "JetStream context is not valid");
        return -1;
    }
    if (qore_check_cancel(xsink)) {
        return -1;
    }

    jsErrCode jerr{};
    natsStatus s = js_DeleteStream(js, name, nullptr, &jerr);
    if (s != NATS_OK) {
        nats_js_error(xsink, "NATS-JETSTREAM-ERROR", s, jerr,
            "failed to delete stream '%s'", name);
        return -1;
    }
    return 0;
}

QoreHashNode* QoreNatsJetStream::getStreamInfo(const char* name, ExceptionSink* xsink) {
    if (!js) {
        xsink->raiseException("NATS-JETSTREAM-ERROR", "JetStream context is not valid");
        return nullptr;
    }
    if (qore_check_cancel(xsink)) {
        return nullptr;
    }

    jsStreamInfo* info = nullptr;
    jsErrCode jerr{};
    natsStatus s = js_GetStreamInfo(&info, js, name, nullptr, &jerr);
    if (s != NATS_OK) {
        nats_js_error(xsink, "NATS-JETSTREAM-ERROR", s, jerr,
            "failed to get stream info for '%s'", name);
        return nullptr;
    }

    QoreHashNode* rv = streamInfoToHash(info, xsink);
    jsStreamInfo_Destroy(info);
    return rv;
}

int QoreNatsJetStream::purgeStream(const char* name, ExceptionSink* xsink) {
    if (!js) {
        xsink->raiseException("NATS-JETSTREAM-ERROR", "JetStream context is not valid");
        return -1;
    }
    if (qore_check_cancel(xsink)) {
        return -1;
    }

    jsErrCode jerr{};
    natsStatus s = js_PurgeStream(js, name, nullptr, &jerr);
    if (s != NATS_OK) {
        nats_js_error(xsink, "NATS-JETSTREAM-ERROR", s, jerr,
            "failed to purge stream '%s'", name);
        return -1;
    }
    return 0;
}

int QoreNatsJetStream::configureConsumerConfig(jsConsumerConfig* cfg,
        const QoreHashNode* config, ExceptionSink* xsink) {
    jsConsumerConfig_Init(cfg);

    QoreValue v = config->getKeyValue("durable_name");
    if (v.getType() == NT_STRING) {
        cfg->Durable = v.get<const QoreStringNode>()->c_str();
    }

    v = config->getKeyValue("deliver_subject");
    if (v.getType() == NT_STRING) {
        cfg->DeliverSubject = v.get<const QoreStringNode>()->c_str();
    }

    v = config->getKeyValue("deliver_group");
    if (v.getType() == NT_STRING) {
        cfg->DeliverGroup = v.get<const QoreStringNode>()->c_str();
    }

    v = config->getKeyValue("description");
    if (v.getType() == NT_STRING) {
        cfg->Description = v.get<const QoreStringNode>()->c_str();
    }

    v = config->getKeyValue("ack_policy");
    if (v.getType() == NT_INT) {
        cfg->AckPolicy = (jsAckPolicy)v.getAsBigInt();
    }

    v = config->getKeyValue("ack_wait");
    if (v.getType() == NT_INT) {
        cfg->AckWait = v.getAsBigInt() * 1000000LL;  // ms to ns
    }

    v = config->getKeyValue("deliver_policy");
    if (v.getType() == NT_INT) {
        cfg->DeliverPolicy = (jsDeliverPolicy)v.getAsBigInt();
    }

    v = config->getKeyValue("opt_start_seq");
    if (v.getType() == NT_INT) {
        cfg->OptStartSeq = (uint64_t)v.getAsBigInt();
    }

    v = config->getKeyValue("replay_policy");
    if (v.getType() == NT_INT) {
        cfg->ReplayPolicy = (jsReplayPolicy)v.getAsBigInt();
    }

    v = config->getKeyValue("filter_subject");
    if (v.getType() == NT_STRING) {
        cfg->FilterSubject = v.get<const QoreStringNode>()->c_str();
    }

    v = config->getKeyValue("max_deliver");
    if (v.getType() == NT_INT) {
        cfg->MaxDeliver = (int)v.getAsBigInt();
    }

    v = config->getKeyValue("max_ack_pending");
    if (v.getType() == NT_INT) {
        cfg->MaxAckPending = (int)v.getAsBigInt();
    }

    v = config->getKeyValue("name");
    if (v.getType() == NT_STRING) {
        cfg->Name = v.get<const QoreStringNode>()->c_str();
    }

    v = config->getKeyValue("back_off");
    if (v.getType() == NT_LIST) {
        const QoreListNode* bo = v.get<const QoreListNode>();
        cfg->BackOffLen = (int)bo->size();
        if (cfg->BackOffLen > 0) {
            cfg->BackOff = (int64_t*)malloc(sizeof(int64_t) * cfg->BackOffLen);
            if (!cfg->BackOff) {
                xsink->raiseException("NATS-JETSTREAM-ERROR",
                    "memory allocation failed for %d back_off entries", cfg->BackOffLen);
                return -1;
            }
            for (int i = 0; i < cfg->BackOffLen; ++i) {
                QoreValue bv = bo->retrieveEntry(i);
                cfg->BackOff[i] = bv.getType() == NT_INT
                    ? bv.getAsBigInt() * 1000000LL : 0;  // ms to ns
            }
        }
    }

    v = config->getKeyValue("rate_limit_bps");
    if (v.getType() == NT_INT) {
        cfg->RateLimit = (uint64_t)v.getAsBigInt();
    }

    v = config->getKeyValue("flow_control");
    if (v.getType() == NT_BOOLEAN) {
        cfg->FlowControl = v.getAsBool();
    }

    v = config->getKeyValue("heartbeat_ms");
    if (v.getType() == NT_INT) {
        cfg->Heartbeat = v.getAsBigInt() * 1000000LL;  // ms to ns
    }

    v = config->getKeyValue("headers_only");
    if (v.getType() == NT_BOOLEAN) {
        cfg->HeadersOnly = v.getAsBool();
    }

    v = config->getKeyValue("max_request_batch");
    if (v.getType() == NT_INT) {
        cfg->MaxRequestBatch = v.getAsBigInt();
    }

    v = config->getKeyValue("max_request_expires_ms");
    if (v.getType() == NT_INT) {
        cfg->MaxRequestExpires = v.getAsBigInt() * 1000000LL;  // ms to ns
    }

    v = config->getKeyValue("max_request_max_bytes");
    if (v.getType() == NT_INT) {
        cfg->MaxRequestMaxBytes = v.getAsBigInt();
    }

    v = config->getKeyValue("inactive_threshold_ms");
    if (v.getType() == NT_INT) {
        cfg->InactiveThreshold = v.getAsBigInt() * 1000000LL;  // ms to ns
    }

    v = config->getKeyValue("num_replicas");
    if (v.getType() == NT_INT) {
        cfg->Replicas = v.getAsBigInt();
    }

    v = config->getKeyValue("memory_storage");
    if (v.getType() == NT_BOOLEAN) {
        cfg->MemoryStorage = v.getAsBool();
    }

    v = config->getKeyValue("filter_subjects");
    if (v.getType() == NT_LIST) {
        const QoreListNode* fs = v.get<const QoreListNode>();
        cfg->FilterSubjectsLen = (int)fs->size();
        if (cfg->FilterSubjectsLen > 0) {
            cfg->FilterSubjects = (const char**)malloc(
                sizeof(const char*) * cfg->FilterSubjectsLen);
            if (!cfg->FilterSubjects) {
                xsink->raiseException("NATS-JETSTREAM-ERROR",
                    "memory allocation failed for %d filter subjects",
                    cfg->FilterSubjectsLen);
                return -1;
            }
            for (int i = 0; i < cfg->FilterSubjectsLen; ++i) {
                QoreValue sv = fs->retrieveEntry(i);
                cfg->FilterSubjects[i] = sv.getType() == NT_STRING
                    ? sv.get<const QoreStringNode>()->c_str() : "";
            }
        }
    }

    v = config->getKeyValue("sample_frequency");
    if (v.getType() == NT_STRING) {
        cfg->SampleFrequency = v.get<const QoreStringNode>()->c_str();
    }

    return 0;
}

QoreHashNode* QoreNatsJetStream::consumerInfoToHash(jsConsumerInfo* info,
        ExceptionSink* xsink) {
    ReferenceHolder<QoreHashNode> h(new QoreHashNode(hashdeclNatsConsumerInfo, xsink), xsink);
    if (*xsink) {
        return nullptr;
    }

    // Config sub-hash
    ReferenceHolder<QoreHashNode> cfg(new QoreHashNode(hashdeclNatsConsumerConfig, xsink), xsink);
    if (*xsink) {
        return nullptr;
    }
    if (info->Config) {
        if (info->Config->Durable) {
            cfg->setKeyValue("durable_name", new QoreStringNode(info->Config->Durable), xsink);
        }
        if (!*xsink && info->Config->DeliverSubject) {
            cfg->setKeyValue("deliver_subject",
                new QoreStringNode(info->Config->DeliverSubject), xsink);
        }
        if (!*xsink && info->Config->DeliverGroup) {
            cfg->setKeyValue("deliver_group",
                new QoreStringNode(info->Config->DeliverGroup), xsink);
        }
        if (!*xsink && info->Config->Description) {
            cfg->setKeyValue("description",
                new QoreStringNode(info->Config->Description), xsink);
        }
        if (!*xsink) {
            cfg->setKeyValue("ack_policy", (int64)info->Config->AckPolicy, xsink);
        }
        if (!*xsink) {
            cfg->setKeyValue("ack_wait", (int64)(info->Config->AckWait / 1000000LL), xsink);
        }
        if (!*xsink) {
            cfg->setKeyValue("deliver_policy", (int64)info->Config->DeliverPolicy, xsink);
        }
        if (!*xsink) {
            cfg->setKeyValue("replay_policy", (int64)info->Config->ReplayPolicy, xsink);
        }
        if (!*xsink && info->Config->FilterSubject) {
            cfg->setKeyValue("filter_subject",
                new QoreStringNode(info->Config->FilterSubject), xsink);
        }
        if (!*xsink) {
            cfg->setKeyValue("max_deliver", (int64)info->Config->MaxDeliver, xsink);
        }
        if (!*xsink) {
            cfg->setKeyValue("max_ack_pending", (int64)info->Config->MaxAckPending, xsink);
        }
        if (!*xsink && info->Config->Name) {
            cfg->setKeyValue("name", new QoreStringNode(info->Config->Name), xsink);
        }
        if (!*xsink) {
            cfg->setKeyValue("rate_limit_bps", (int64)info->Config->RateLimit, xsink);
        }
        if (!*xsink) {
            cfg->setKeyValue("flow_control", info->Config->FlowControl, xsink);
        }
        if (!*xsink) {
            cfg->setKeyValue("heartbeat_ms",
                (int64)(info->Config->Heartbeat / 1000000LL), xsink);
        }
        if (!*xsink) {
            cfg->setKeyValue("headers_only", info->Config->HeadersOnly, xsink);
        }
        if (!*xsink) {
            cfg->setKeyValue("max_request_batch", (int64)info->Config->MaxRequestBatch, xsink);
        }
        if (!*xsink) {
            cfg->setKeyValue("max_request_expires_ms",
                (int64)(info->Config->MaxRequestExpires / 1000000LL), xsink);
        }
        if (!*xsink) {
            cfg->setKeyValue("max_request_max_bytes",
                (int64)info->Config->MaxRequestMaxBytes, xsink);
        }
        if (!*xsink) {
            cfg->setKeyValue("inactive_threshold_ms",
                (int64)(info->Config->InactiveThreshold / 1000000LL), xsink);
        }
        if (!*xsink) {
            cfg->setKeyValue("num_replicas", (int64)info->Config->Replicas, xsink);
        }
        if (!*xsink) {
            cfg->setKeyValue("memory_storage", info->Config->MemoryStorage, xsink);
        }
        if (!*xsink && info->Config->SampleFrequency) {
            cfg->setKeyValue("sample_frequency",
                new QoreStringNode(info->Config->SampleFrequency), xsink);
        }
        if (*xsink) {
            return nullptr;
        }
    }
    h->setKeyValue("config", cfg.release(), xsink);

    if (!*xsink) {
        h->setKeyValue("delivered", (int64)info->Delivered.Consumer, xsink);
    }
    if (!*xsink) {
        h->setKeyValue("ack_pending", (int64)info->AckFloor.Consumer, xsink);
    }
    if (!*xsink) {
        h->setKeyValue("num_pending", (int64)info->NumPending, xsink);
    }
    if (!*xsink) {
        h->setKeyValue("redelivered", (int64)info->NumRedelivered, xsink);
    }
    if (!*xsink) {
        h->setKeyValue("waiting", (int64)info->NumWaiting, xsink);
    }
    if (*xsink) {
        return nullptr;
    }

    return h.release();
}

QoreHashNode* QoreNatsJetStream::addConsumer(const char* stream,
        const QoreHashNode* config, ExceptionSink* xsink) {
    if (!js) {
        xsink->raiseException("NATS-JETSTREAM-ERROR", "JetStream context is not valid");
        return nullptr;
    }
    if (qore_check_cancel(xsink)) {
        return nullptr;
    }

    jsConsumerConfig cfg;
    configureConsumerConfig(&cfg, config, xsink);
    if (*xsink) {
        if (cfg.BackOff) {
            free((void*)cfg.BackOff);
        }
        if (cfg.FilterSubjects) {
            free((void*)cfg.FilterSubjects);
        }
        return nullptr;
    }

    jsConsumerInfo* info = nullptr;
    jsErrCode jerr{};
    natsStatus s = js_AddConsumer(&info, js, stream, &cfg, nullptr, &jerr);

    if (cfg.BackOff) {
        free((void*)cfg.BackOff);
    }
    if (cfg.FilterSubjects) {
        free((void*)cfg.FilterSubjects);
    }

    if (s != NATS_OK) {
        nats_js_error(xsink, "NATS-JETSTREAM-ERROR", s, jerr,
            "failed to add consumer to stream '%s'", stream);
        return nullptr;
    }

    QoreHashNode* rv = consumerInfoToHash(info, xsink);
    jsConsumerInfo_Destroy(info);
    return rv;
}

int QoreNatsJetStream::deleteConsumer(const char* stream, const char* consumer,
        ExceptionSink* xsink) {
    if (!js) {
        xsink->raiseException("NATS-JETSTREAM-ERROR", "JetStream context is not valid");
        return -1;
    }
    if (qore_check_cancel(xsink)) {
        return -1;
    }

    jsErrCode jerr{};
    natsStatus s = js_DeleteConsumer(js, stream, consumer, nullptr, &jerr);
    if (s != NATS_OK) {
        nats_js_error(xsink, "NATS-JETSTREAM-ERROR", s, jerr,
            "failed to delete consumer '%s' from stream '%s'", consumer, stream);
        return -1;
    }
    return 0;
}

QoreHashNode* QoreNatsJetStream::getConsumerInfo(const char* stream,
        const char* consumer, ExceptionSink* xsink) {
    if (!js) {
        xsink->raiseException("NATS-JETSTREAM-ERROR", "JetStream context is not valid");
        return nullptr;
    }
    if (qore_check_cancel(xsink)) {
        return nullptr;
    }

    jsConsumerInfo* info = nullptr;
    jsErrCode jerr{};
    natsStatus s = js_GetConsumerInfo(&info, js, stream, consumer, nullptr, &jerr);
    if (s != NATS_OK) {
        nats_js_error(xsink, "NATS-JETSTREAM-ERROR", s, jerr,
            "failed to get consumer info for '%s' on stream '%s'", consumer, stream);
        return nullptr;
    }

    QoreHashNode* rv = consumerInfoToHash(info, xsink);
    jsConsumerInfo_Destroy(info);
    return rv;
}

QoreHashNode* QoreNatsJetStream::publish(const char* subject, const void* data,
        int data_len, ExceptionSink* xsink) {
    if (!js) {
        xsink->raiseException("NATS-JETSTREAM-ERROR", "JetStream context is not valid");
        return nullptr;
    }
    if (qore_check_cancel(xsink)) {
        return nullptr;
    }

    jsPubAck* pa = nullptr;
    jsErrCode jerr{};
    natsStatus s = js_Publish(&pa, js, subject, data, data_len, nullptr, &jerr);
    if (s != NATS_OK) {
        nats_js_error(xsink, "NATS-JETSTREAM-ERROR", s, jerr,
            "failed to publish to subject '%s'", subject);
        return nullptr;
    }

    ReferenceHolder<QoreHashNode> h(new QoreHashNode(hashdeclNatsPubAck, xsink), xsink);
    if (pa->Stream) {
        h->setKeyValue("stream", new QoreStringNode(pa->Stream), xsink);
    }
    h->setKeyValue("sequence", (int64)pa->Sequence, xsink);
    h->setKeyValue("duplicate", pa->Duplicate, xsink);
    jsPubAck_Destroy(pa);

    return h.release();
}

QoreNatsSubscription* QoreNatsJetStream::subscribe(const char* subject,
        ExceptionSink* xsink) {
    if (!js) {
        xsink->raiseException("NATS-JETSTREAM-ERROR", "JetStream context is not valid");
        return nullptr;
    }
    if (qore_check_cancel(xsink)) {
        return nullptr;
    }

    natsSubscription* sub = nullptr;
    jsErrCode jerr{};
    natsStatus s = js_SubscribeSync(&sub, js, subject, nullptr, nullptr, &jerr);
    if (s != NATS_OK) {
        nats_js_error(xsink, "NATS-JETSTREAM-ERROR", s, jerr,
            "failed to subscribe to JetStream subject '%s'", subject);
        return nullptr;
    }
    return new QoreNatsSubscription(sub);
}

QoreNatsSubscription* QoreNatsJetStream::pullSubscribe(const char* subject,
        const char* durable, ExceptionSink* xsink) {
    if (!js) {
        xsink->raiseException("NATS-JETSTREAM-ERROR", "JetStream context is not valid");
        return nullptr;
    }
    if (qore_check_cancel(xsink)) {
        return nullptr;
    }

    natsSubscription* sub = nullptr;
    jsErrCode jerr{};
    natsStatus s = js_PullSubscribe(&sub, js, subject, durable, nullptr, nullptr, &jerr);
    if (s != NATS_OK) {
        nats_js_error(xsink, "NATS-JETSTREAM-ERROR", s, jerr,
            "failed to pull subscribe to JetStream subject '%s' durable '%s'",
            subject, durable);
        return nullptr;
    }
    return new QoreNatsSubscription(sub);
}

QoreHashNode* QoreNatsJetStream::publishWithOptions(const char* subject, const void* data,
        int data_len, const QoreHashNode* pub_opts, ExceptionSink* xsink) {
    if (!js) {
        xsink->raiseException("NATS-JETSTREAM-ERROR", "JetStream context is not valid");
        return nullptr;
    }
    if (qore_check_cancel(xsink)) {
        return nullptr;
    }

    jsPubOptions po;
    jsPubOptions_Init(&po);

    QoreValue v = pub_opts->getKeyValue("max_wait_ms");
    if (v.getType() == NT_INT) {
        po.MaxWait = v.getAsBigInt();
    }

    v = pub_opts->getKeyValue("msg_id");
    if (v.getType() == NT_STRING) {
        po.MsgId = v.get<const QoreStringNode>()->c_str();
    }

    v = pub_opts->getKeyValue("expect_stream");
    if (v.getType() == NT_STRING) {
        po.ExpectStream = v.get<const QoreStringNode>()->c_str();
    }

    v = pub_opts->getKeyValue("expect_last_msg_id");
    if (v.getType() == NT_STRING) {
        po.ExpectLastMsgId = v.get<const QoreStringNode>()->c_str();
    }

    v = pub_opts->getKeyValue("expect_last_seq");
    if (v.getType() == NT_INT) {
        po.ExpectLastSeq = (uint64_t)v.getAsBigInt();
    }

    v = pub_opts->getKeyValue("expect_last_subject_seq");
    if (v.getType() == NT_INT) {
        po.ExpectLastSubjectSeq = (uint64_t)v.getAsBigInt();
    }

    v = pub_opts->getKeyValue("expect_no_message");
    if (v.getType() == NT_BOOLEAN) {
        po.ExpectNoMessage = v.getAsBool();
    }

    jsPubAck* pa = nullptr;
    jsErrCode jerr{};
    natsStatus s = js_Publish(&pa, js, subject, data, data_len, &po, &jerr);
    if (s != NATS_OK) {
        nats_js_error(xsink, "NATS-JETSTREAM-ERROR", s, jerr,
            "failed to publish to subject '%s'", subject);
        return nullptr;
    }

    ReferenceHolder<QoreHashNode> h(new QoreHashNode(hashdeclNatsPubAck, xsink), xsink);
    if (pa->Stream) {
        h->setKeyValue("stream", new QoreStringNode(pa->Stream), xsink);
    }
    if (!*xsink) {
        h->setKeyValue("sequence", (int64)pa->Sequence, xsink);
    }
    if (!*xsink) {
        h->setKeyValue("duplicate", pa->Duplicate, xsink);
    }
    jsPubAck_Destroy(pa);

    if (*xsink) {
        return nullptr;
    }
    return h.release();
}

QoreHashNode* QoreNatsJetStream::getAccountInfo(ExceptionSink* xsink) {
    if (!js) {
        xsink->raiseException("NATS-JETSTREAM-ERROR", "JetStream context is not valid");
        return nullptr;
    }
    if (qore_check_cancel(xsink)) {
        return nullptr;
    }

    jsAccountInfo* ai = nullptr;
    jsErrCode jerr{};
    natsStatus s = js_GetAccountInfo(&ai, js, nullptr, &jerr);
    if (s != NATS_OK) {
        nats_js_error(xsink, "NATS-JETSTREAM-ERROR", s, jerr,
            "failed to get account info");
        return nullptr;
    }

    ReferenceHolder<QoreHashNode> h(new QoreHashNode(hashdeclNatsJSAccountInfo, xsink), xsink);
    if (*xsink) {
        jsAccountInfo_Destroy(ai);
        return nullptr;
    }

    h->setKeyValue("memory", (int64)ai->Memory, xsink);
    if (!*xsink) {
        h->setKeyValue("storage", (int64)ai->Store, xsink);
    }
    if (!*xsink) {
        h->setKeyValue("streams", (int64)ai->Streams, xsink);
    }
    if (!*xsink) {
        h->setKeyValue("consumers", (int64)ai->Consumers, xsink);
    }
    if (!*xsink && ai->Domain) {
        h->setKeyValue("domain", new QoreStringNode(ai->Domain), xsink);
    }
    if (!*xsink) {
        h->setKeyValue("api_total", (int64)ai->API.Total, xsink);
    }
    if (!*xsink) {
        h->setKeyValue("api_errors", (int64)ai->API.Errors, xsink);
    }

    // Build limits sub-hash
    if (!*xsink) {
        ReferenceHolder<QoreHashNode> limits(new QoreHashNode(hashdeclNatsJSAccountLimits, xsink), xsink);
        if (!*xsink) {
            limits->setKeyValue("max_memory", (int64)ai->Limits.MaxMemory, xsink);
        }
        if (!*xsink) {
            limits->setKeyValue("max_storage", (int64)ai->Limits.MaxStore, xsink);
        }
        if (!*xsink) {
            limits->setKeyValue("max_streams", (int64)ai->Limits.MaxStreams, xsink);
        }
        if (!*xsink) {
            limits->setKeyValue("max_consumers", (int64)ai->Limits.MaxConsumers, xsink);
        }
        if (!*xsink) {
            limits->setKeyValue("max_ack_pending", (int64)ai->Limits.MaxAckPending, xsink);
        }
        if (!*xsink) {
            limits->setKeyValue("memory_max_stream_bytes",
                (int64)ai->Limits.MemoryMaxStreamBytes, xsink);
        }
        if (!*xsink) {
            limits->setKeyValue("storage_max_stream_bytes",
                (int64)ai->Limits.StoreMaxStreamBytes, xsink);
        }
        if (!*xsink) {
            limits->setKeyValue("max_bytes_required", ai->Limits.MaxBytesRequired, xsink);
        }
        if (!*xsink) {
            h->setKeyValue("limits", limits.release(), xsink);
        }
    }

    jsAccountInfo_Destroy(ai);

    if (*xsink) {
        return nullptr;
    }
    return h.release();
}

QoreNatsKVStore* QoreNatsJetStream::keyValue(const char* bucket, ExceptionSink* xsink) {
    if (!js) {
        xsink->raiseException("NATS-KV-ERROR", "JetStream context is not valid");
        return nullptr;
    }
    if (qore_check_cancel(xsink)) {
        return nullptr;
    }

    kvStore* kv = nullptr;
    natsStatus s = js_KeyValue(&kv, js, bucket);
    if (s != NATS_OK) {
        nats_error(xsink, "NATS-KV-ERROR", s,
            "failed to open KV bucket '%s'", bucket);
        return nullptr;
    }
    return new QoreNatsKVStore(kv);
}

QoreNatsKVStore* QoreNatsJetStream::createKeyValue(const QoreHashNode* config,
        ExceptionSink* xsink) {
    if (!js) {
        xsink->raiseException("NATS-KV-ERROR", "JetStream context is not valid");
        return nullptr;
    }
    if (qore_check_cancel(xsink)) {
        return nullptr;
    }

    kvConfig kvc;
    memset(&kvc, 0, sizeof(kvc));

    QoreValue v = config->getKeyValue("bucket");
    if (v.getType() == NT_STRING) {
        kvc.Bucket = v.get<const QoreStringNode>()->c_str();
    }

    v = config->getKeyValue("description");
    if (v.getType() == NT_STRING) {
        kvc.Description = v.get<const QoreStringNode>()->c_str();
    }

    v = config->getKeyValue("max_value_size");
    if (v.getType() == NT_INT) {
        kvc.MaxValueSize = (int32_t)v.getAsBigInt();
    }

    v = config->getKeyValue("history");
    if (v.getType() == NT_INT) {
        kvc.History = (int)v.getAsBigInt();
    }

    v = config->getKeyValue("ttl");
    if (v.getType() == NT_INT) {
        kvc.TTL = v.getAsBigInt() * 1000000LL;  // ms to ns
    }

    v = config->getKeyValue("max_bytes");
    if (v.getType() == NT_INT) {
        kvc.MaxBytes = v.getAsBigInt();
    }

    v = config->getKeyValue("storage");
    if (v.getType() == NT_INT) {
        kvc.StorageType = (jsStorageType)v.getAsBigInt();
    }

    v = config->getKeyValue("num_replicas");
    if (v.getType() == NT_INT) {
        kvc.Replicas = (int)v.getAsBigInt();
    }

    kvStore* kv = nullptr;
    natsStatus s = js_CreateKeyValue(&kv, js, &kvc);
    if (s != NATS_OK) {
        nats_error(xsink, "NATS-KV-ERROR", s, "failed to create KV bucket");
        return nullptr;
    }
    return new QoreNatsKVStore(kv);
}

int QoreNatsJetStream::deleteKeyValue(const char* bucket, ExceptionSink* xsink) {
    if (!js) {
        xsink->raiseException("NATS-KV-ERROR", "JetStream context is not valid");
        return -1;
    }
    if (qore_check_cancel(xsink)) {
        return -1;
    }

    natsStatus s = js_DeleteKeyValue(js, bucket);
    if (s != NATS_OK) {
        nats_error(xsink, "NATS-KV-ERROR", s,
            "failed to delete KV bucket '%s'", bucket);
        return -1;
    }
    return 0;
}
