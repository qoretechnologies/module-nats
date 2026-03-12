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
    if (info->Config) {
        if (info->Config->Name) {
            cfg->setKeyValue("name", new QoreStringNode(info->Config->Name), xsink);
        }
        if (info->Config->Description) {
            cfg->setKeyValue("description", new QoreStringNode(info->Config->Description), xsink);
        }
        cfg->setKeyValue("retention", (int64)info->Config->Retention, xsink);
        cfg->setKeyValue("max_msgs", (int64)info->Config->MaxMsgs, xsink);
        cfg->setKeyValue("max_bytes", (int64)info->Config->MaxBytes, xsink);
        cfg->setKeyValue("max_age", (int64)(info->Config->MaxAge / 1000000LL), xsink);
        cfg->setKeyValue("max_msg_size", (int64)info->Config->MaxMsgSize, xsink);
        cfg->setKeyValue("storage", (int64)info->Config->Storage, xsink);
        cfg->setKeyValue("num_replicas", (int64)info->Config->Replicas, xsink);
        cfg->setKeyValue("discard", (int64)info->Config->Discard, xsink);

        // Subjects
        if (info->Config->SubjectsLen > 0 && info->Config->Subjects) {
            ReferenceHolder<QoreListNode> subjects(new QoreListNode(stringTypeInfo), xsink);
            for (int i = 0; i < info->Config->SubjectsLen; ++i) {
                subjects->push(new QoreStringNode(info->Config->Subjects[i]), xsink);
            }
            cfg->setKeyValue("subjects", subjects.release(), xsink);
        }
    }
    h->setKeyValue("config", cfg.release(), xsink);

    h->setKeyValue("messages", (int64)info->State.Msgs, xsink);
    h->setKeyValue("bytes", (int64)info->State.Bytes, xsink);
    h->setKeyValue("first_seq", (int64)info->State.FirstSeq, xsink);
    h->setKeyValue("last_seq", (int64)info->State.LastSeq, xsink);
    h->setKeyValue("consumer_count", (int64)info->State.Consumers, xsink);

    return h.release();
}

QoreHashNode* QoreNatsJetStream::addStream(const QoreHashNode* config,
        ExceptionSink* xsink) {
    if (!js) {
        xsink->raiseException("NATS-JETSTREAM-ERROR", "JetStream context is not valid");
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
    jsErrCode jerr = 0;
    natsStatus s = js_AddStream(&info, js, &cfg, nullptr, &jerr);

    if (cfg.Subjects) {
        free((void*)cfg.Subjects);
    }

    if (s != NATS_OK) {
        nats_error(xsink, "NATS-JETSTREAM-ERROR", s, "failed to add stream");
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

    jsStreamConfig cfg;
    configureStreamConfig(&cfg, config, xsink);
    if (*xsink) {
        if (cfg.Subjects) {
            free((void*)cfg.Subjects);
        }
        return nullptr;
    }

    jsStreamInfo* info = nullptr;
    jsErrCode jerr = 0;
    natsStatus s = js_UpdateStream(&info, js, &cfg, nullptr, &jerr);

    if (cfg.Subjects) {
        free((void*)cfg.Subjects);
    }

    if (s != NATS_OK) {
        nats_error(xsink, "NATS-JETSTREAM-ERROR", s, "failed to update stream");
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

    jsErrCode jerr = 0;
    natsStatus s = js_DeleteStream(js, name, nullptr, &jerr);
    if (s != NATS_OK) {
        nats_error(xsink, "NATS-JETSTREAM-ERROR", s,
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

    jsStreamInfo* info = nullptr;
    jsErrCode jerr = 0;
    natsStatus s = js_GetStreamInfo(&info, js, name, nullptr, &jerr);
    if (s != NATS_OK) {
        nats_error(xsink, "NATS-JETSTREAM-ERROR", s,
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

    jsErrCode jerr = 0;
    natsStatus s = js_PurgeStream(js, name, nullptr, &jerr);
    if (s != NATS_OK) {
        nats_error(xsink, "NATS-JETSTREAM-ERROR", s,
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
    if (info->Config) {
        if (info->Config->Durable) {
            cfg->setKeyValue("durable_name", new QoreStringNode(info->Config->Durable), xsink);
        }
        if (info->Config->DeliverSubject) {
            cfg->setKeyValue("deliver_subject",
                new QoreStringNode(info->Config->DeliverSubject), xsink);
        }
        if (info->Config->DeliverGroup) {
            cfg->setKeyValue("deliver_group",
                new QoreStringNode(info->Config->DeliverGroup), xsink);
        }
        if (info->Config->Description) {
            cfg->setKeyValue("description",
                new QoreStringNode(info->Config->Description), xsink);
        }
        cfg->setKeyValue("ack_policy", (int64)info->Config->AckPolicy, xsink);
        cfg->setKeyValue("ack_wait", (int64)(info->Config->AckWait / 1000000LL), xsink);
        cfg->setKeyValue("deliver_policy", (int64)info->Config->DeliverPolicy, xsink);
        cfg->setKeyValue("replay_policy", (int64)info->Config->ReplayPolicy, xsink);
        if (info->Config->FilterSubject) {
            cfg->setKeyValue("filter_subject",
                new QoreStringNode(info->Config->FilterSubject), xsink);
        }
        cfg->setKeyValue("max_deliver", (int64)info->Config->MaxDeliver, xsink);
        cfg->setKeyValue("max_ack_pending", (int64)info->Config->MaxAckPending, xsink);
    }
    h->setKeyValue("config", cfg.release(), xsink);

    h->setKeyValue("delivered", (int64)info->Delivered.Consumer, xsink);
    h->setKeyValue("ack_pending", (int64)info->AckFloor.Consumer, xsink);
    h->setKeyValue("num_pending", (int64)info->NumPending, xsink);
    h->setKeyValue("redelivered", (int64)info->NumRedelivered, xsink);
    h->setKeyValue("waiting", (int64)info->NumWaiting, xsink);

    return h.release();
}

QoreHashNode* QoreNatsJetStream::addConsumer(const char* stream,
        const QoreHashNode* config, ExceptionSink* xsink) {
    if (!js) {
        xsink->raiseException("NATS-JETSTREAM-ERROR", "JetStream context is not valid");
        return nullptr;
    }

    jsConsumerConfig cfg;
    configureConsumerConfig(&cfg, config, xsink);
    if (*xsink) {
        return nullptr;
    }

    jsConsumerInfo* info = nullptr;
    jsErrCode jerr = 0;
    natsStatus s = js_AddConsumer(&info, js, stream, &cfg, nullptr, &jerr);
    if (s != NATS_OK) {
        nats_error(xsink, "NATS-JETSTREAM-ERROR", s,
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

    jsErrCode jerr = 0;
    natsStatus s = js_DeleteConsumer(js, stream, consumer, nullptr, &jerr);
    if (s != NATS_OK) {
        nats_error(xsink, "NATS-JETSTREAM-ERROR", s,
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

    jsConsumerInfo* info = nullptr;
    jsErrCode jerr = 0;
    natsStatus s = js_GetConsumerInfo(&info, js, stream, consumer, nullptr, &jerr);
    if (s != NATS_OK) {
        nats_error(xsink, "NATS-JETSTREAM-ERROR", s,
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

    jsPubAck* pa = nullptr;
    jsErrCode jerr = 0;
    natsStatus s = js_Publish(&pa, js, subject, data, data_len, nullptr, &jerr);
    if (s != NATS_OK) {
        nats_error(xsink, "NATS-JETSTREAM-ERROR", s,
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

    natsSubscription* sub = nullptr;
    jsErrCode jerr = 0;
    natsStatus s = js_Subscribe(&sub, js, subject, nullptr, nullptr, nullptr, nullptr, &jerr);
    if (s != NATS_OK) {
        nats_error(xsink, "NATS-JETSTREAM-ERROR", s,
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

    natsSubscription* sub = nullptr;
    jsErrCode jerr = 0;
    natsStatus s = js_PullSubscribe(&sub, js, subject, durable, nullptr, nullptr, &jerr);
    if (s != NATS_OK) {
        nats_error(xsink, "NATS-JETSTREAM-ERROR", s,
            "failed to pull subscribe to JetStream subject '%s' durable '%s'",
            subject, durable);
        return nullptr;
    }
    return new QoreNatsSubscription(sub);
}

QoreNatsKVStore* QoreNatsJetStream::keyValue(const char* bucket, ExceptionSink* xsink) {
    if (!js) {
        xsink->raiseException("NATS-KV-ERROR", "JetStream context is not valid");
        return nullptr;
    }

    kvStore* kv = nullptr;
    jsErrCode jerr = 0;
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

    natsStatus s = js_DeleteKeyValue(js, bucket);
    if (s != NATS_OK) {
        nats_error(xsink, "NATS-KV-ERROR", s,
            "failed to delete KV bucket '%s'", bucket);
        return -1;
    }
    return 0;
}
