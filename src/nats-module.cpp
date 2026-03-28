/* -*- mode: c++; indent-tabs-mode: nil -*- */
/** @file nats-module.cpp nats module implementation */
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

#include "nats-module.h"
#include "QC_NatsConnection.h"
#include "QC_NatsSubscription.h"
#include "QC_JetStreamContext.h"
#include "QC_NatsKeyValueWatcher.h"
#include "QC_NatsKeyValueStore.h"
#include "QC_NatsMicroService.h"
#include "QC_NatsMicroGroup.h"

static void nats_module_init(QoreModuleInitContext& ctx, ExceptionSink& xsink);
static void nats_module_ns_init(QoreNamespace* rns, QoreNamespace* qns, ExceptionSink& xsink);
static void nats_module_delete();

// Defined in generated QC_NatsMsg.cpp
DLLLOCAL void init_NatsMsg_constants(QoreNamespace& ns);

extern "C" DLLEXPORT void nats_qore_module_desc(QoreModuleInfo& mod_info) {
    mod_info.name = "nats";
    mod_info.version = "1.0.0";
    mod_info.desc = "Qore NATS messaging module";
    mod_info.author = "Qore Technologies, s.r.o.";
    mod_info.url = "https://github.com/qoretechnologies/module-nats";
    mod_info.api_major = QORE_MODULE_API_MAJOR;
    mod_info.api_minor = QORE_MODULE_API_MINOR;
    mod_info.init = nats_module_init;
    mod_info.ns_init = nats_module_ns_init;
    mod_info.del = nats_module_delete;
    mod_info.license = QL_MIT;
    mod_info.license_str = "MIT";
}

// Global hashdecl pointers
const TypedHashDecl* hashdeclNatsTlsOptions = nullptr;
const TypedHashDecl* hashdeclNatsConnectionOptions = nullptr;
const TypedHashDecl* hashdeclNatsMsgInfo = nullptr;
const TypedHashDecl* hashdeclNatsStreamConfig = nullptr;
const TypedHashDecl* hashdeclNatsConsumerConfig = nullptr;
const TypedHashDecl* hashdeclNatsStreamInfo = nullptr;
const TypedHashDecl* hashdeclNatsConsumerInfo = nullptr;
const TypedHashDecl* hashdeclNatsPubAck = nullptr;
const TypedHashDecl* hashdeclNatsKVConfig = nullptr;
const TypedHashDecl* hashdeclNatsKVEntry = nullptr;
const TypedHashDecl* hashdeclNatsConnectionStats = nullptr;
const TypedHashDecl* hashdeclNatsConnectionInfo = nullptr;
const TypedHashDecl* hashdeclNatsJSMsgMetadata = nullptr;
const TypedHashDecl* hashdeclNatsJSPubOptions = nullptr;
const TypedHashDecl* hashdeclNatsJSAccountLimits = nullptr;
const TypedHashDecl* hashdeclNatsJSAccountInfo = nullptr;
const TypedHashDecl* hashdeclNatsJSSubOptions = nullptr;
const TypedHashDecl* hashdeclNatsSubscriptionStats = nullptr;
const TypedHashDecl* hashdeclNatsKVBucketStatus = nullptr;
const TypedHashDecl* hashdeclNatsKVWatchOptions = nullptr;
const TypedHashDecl* hashdeclNatsMicroEndpointConfig = nullptr;
const TypedHashDecl* hashdeclNatsMicroServiceConfig = nullptr;
const TypedHashDecl* hashdeclNatsMicroEndpointInfo = nullptr;
const TypedHashDecl* hashdeclNatsMicroServiceInfo = nullptr;
const TypedHashDecl* hashdeclNatsMicroEndpointStats = nullptr;
const TypedHashDecl* hashdeclNatsMicroServiceStats = nullptr;

QoreNamespace NatsNs("Qore::Nats");

static void nats_module_init(QoreModuleInitContext& ctx, ExceptionSink& xsink) {
    // Initialize constants
    init_NatsMsg_constants(NatsNs);

    // Initialize Phase 2 hashdecls (Core NATS)
    hashdeclNatsTlsOptions = init_hashdecl_NatsTlsOptions(NatsNs);
    hashdeclNatsConnectionOptions = init_hashdecl_NatsConnectionOptions(NatsNs);
    hashdeclNatsMsgInfo = init_hashdecl_NatsMsgInfo(NatsNs);

    // Initialize Phase 3 hashdecls (JetStream) - must be before classes that reference them
    hashdeclNatsStreamConfig = init_hashdecl_NatsStreamConfig(NatsNs);
    hashdeclNatsConsumerConfig = init_hashdecl_NatsConsumerConfig(NatsNs);
    hashdeclNatsStreamInfo = init_hashdecl_NatsStreamInfo(NatsNs);
    hashdeclNatsConsumerInfo = init_hashdecl_NatsConsumerInfo(NatsNs);
    hashdeclNatsPubAck = init_hashdecl_NatsPubAck(NatsNs);
    hashdeclNatsKVConfig = init_hashdecl_NatsKVConfig(NatsNs);
    hashdeclNatsKVEntry = init_hashdecl_NatsKVEntry(NatsNs);

    // Initialize Phase 4 hashdecls (Connection introspection)
    hashdeclNatsConnectionStats = init_hashdecl_NatsConnectionStats(NatsNs);
    hashdeclNatsConnectionInfo = init_hashdecl_NatsConnectionInfo(NatsNs);

    // Initialize Phase 5 hashdecls (JetStream metadata and publish options)
    hashdeclNatsJSMsgMetadata = init_hashdecl_NatsJSMsgMetadata(NatsNs);
    hashdeclNatsJSPubOptions = init_hashdecl_NatsJSPubOptions(NatsNs);
    hashdeclNatsJSAccountLimits = init_hashdecl_NatsJSAccountLimits(NatsNs);
    hashdeclNatsJSAccountInfo = init_hashdecl_NatsJSAccountInfo(NatsNs);
    hashdeclNatsJSSubOptions = init_hashdecl_NatsJSSubOptions(NatsNs);

    // Initialize Phase 4 hashdecls (Monitoring & Statistics)
    hashdeclNatsSubscriptionStats = init_hashdecl_NatsSubscriptionStats(NatsNs);
    hashdeclNatsKVBucketStatus = init_hashdecl_NatsKVBucketStatus(NatsNs);
    hashdeclNatsKVWatchOptions = init_hashdecl_NatsKVWatchOptions(NatsNs);

    // Initialize Phase 7 hashdecls (Microservices)
    hashdeclNatsMicroEndpointConfig = init_hashdecl_NatsMicroEndpointConfig(NatsNs);
    hashdeclNatsMicroServiceConfig = init_hashdecl_NatsMicroServiceConfig(NatsNs);
    hashdeclNatsMicroEndpointInfo = init_hashdecl_NatsMicroEndpointInfo(NatsNs);
    hashdeclNatsMicroServiceInfo = init_hashdecl_NatsMicroServiceInfo(NatsNs);
    hashdeclNatsMicroEndpointStats = init_hashdecl_NatsMicroEndpointStats(NatsNs);
    hashdeclNatsMicroServiceStats = init_hashdecl_NatsMicroServiceStats(NatsNs);

    // Initialize classes in dependency order:
    // NatsSubscription has no class deps
    NatsNs.addSystemClass(initNatsSubscriptionClass(NatsNs));
    // NatsKeyValueWatcher has no class deps
    NatsNs.addSystemClass(initNatsKeyValueWatcherClass(NatsNs));
    // NatsKeyValueStore depends on NatsKeyValueWatcher (watch methods return it)
    NatsNs.addSystemClass(initNatsKeyValueStoreClass(NatsNs));
    // JetStreamContext depends on NatsSubscription, NatsKeyValueStore
    NatsNs.addSystemClass(initJetStreamContextClass(NatsNs));
    // NatsMicroGroup has no class deps
    NatsNs.addSystemClass(initNatsMicroGroupClass(NatsNs));
    // NatsMicroService depends on NatsMicroGroup (addGroup returns it)
    NatsNs.addSystemClass(initNatsMicroServiceClass(NatsNs));
    // NatsConnection depends on NatsSubscription, JetStreamContext, NatsMicroService
    NatsNs.addSystemClass(initNatsConnectionClass(NatsNs));
}

static void nats_module_ns_init(QoreNamespace* rns, QoreNamespace* qns, ExceptionSink& xsink) {
    qns->addNamespace(NatsNs.copy());
}

static void nats_module_delete() {
    // nats.c library cleanup; wait for async operations to finish
    nats_CloseAndWait(5000);
}
