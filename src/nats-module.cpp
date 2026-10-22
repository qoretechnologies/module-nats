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
const TypedHashDecl* hashdeclNatsJSConsumerPauseResponse = nullptr;
const TypedHashDecl* hashdeclNatsMicroEndpointConfig = nullptr;
const TypedHashDecl* hashdeclNatsMicroServiceConfig = nullptr;
const TypedHashDecl* hashdeclNatsMicroEndpointInfo = nullptr;
const TypedHashDecl* hashdeclNatsMicroServiceInfo = nullptr;
const TypedHashDecl* hashdeclNatsMicroEndpointStats = nullptr;
const TypedHashDecl* hashdeclNatsMicroServiceStats = nullptr;

QoreNamespace NatsNs("Qore::Nats");

//! the process that loaded the module (see nats_module_delete())
static pid_t nats_module_pid = 0;

#ifdef _QORE_HAS_FORK_CHILD_QUICK_EXIT
//! true once the C client library has been used in the process (or in the parent of a child created with fork())
static std::atomic<bool> nats_library_opened{false};

//! the registration of nats_fork_child()
static QoreForkHandlerRegistration* nats_fork_reg = nullptr;

// runs in the child of fork(): once the C client library is open, its atexit() handler (natsLib_Destructor()) takes
// the library lock, which a thread of the client in the parent can hold when the process is copied, so that the
// child would wait forever when it exits; such a child ends without running exit-time handlers (see
// QoreForkHandler.h).  Takes no lock and allocates nothing.
static void nats_fork_child(void*) {
    if (nats_library_opened.load(std::memory_order_relaxed)) {
        qore_fork_child_exit_quickly();
    }
}
#endif

void nats_library_used() {
#ifdef _QORE_HAS_FORK_CHILD_QUICK_EXIT
    nats_library_opened.store(true, std::memory_order_relaxed);
#endif
}

#ifdef DEBUG
// the state of the C client library (nats.c src/glib/glibp.h): its first member is the library lock, a natsMutex,
// which is a pthread_mutex_t on POSIX systems; not part of the client's public API
extern "C" void* nats_lib(void);

//! Debug-only: calls the code while the calling thread holds the C client's library lock
/** Lets the tests create a child process with fork() while another thread holds the lock, as a thread of the C
    client can when another thread forks: the lock stays held forever in the child.  DEBUG builds only.

    @throw NATS-DEBUG-ERROR the C client library has not been opened (no NATS object has been created)
*/
static QoreValue f_dbg_with_library_lock(const QoreListNode* args, RuntimeConfig& rc, ExceptionSink* xsink) {
    const ResolvedCallReferenceNode* code = get_param_value(args, 0).get<const ResolvedCallReferenceNode>();
    pthread_mutex_t* lock = *static_cast<pthread_mutex_t**>(nats_lib());
    if (!lock) {
        xsink->raiseException("NATS-DEBUG-ERROR", "the NATS C client library has not been opened");
        return QoreValue();
    }
    pthread_mutex_lock(lock);
    ON_BLOCK_EXIT(pthread_mutex_unlock, lock);
    return code->execValue(nullptr, xsink);
}
#endif

static void nats_module_init(QoreModuleInitContext& ctx, ExceptionSink& xsink) {
    nats_module_pid = getpid();
#ifdef _QORE_HAS_FORK_CHILD_QUICK_EXIT
    // registered in the initialization function, as fork handlers must be; nothing below fails, so it is removed
    // only when the module is deleted
    nats_fork_reg = qore_register_fork_handler(QFR_MODULE_LEAF, "nats_exit", nullptr, nullptr, nats_fork_child,
        nullptr);
    if (!nats_fork_reg) {
        xsink.outOfMemory();
        return;
    }
#endif

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
    hashdeclNatsJSConsumerPauseResponse = init_hashdecl_NatsJSConsumerPauseResponse(NatsNs);

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

#ifdef DEBUG
    NatsNs.addBuiltinVariant("dbg_with_library_lock", f_dbg_with_library_lock, QCF_NO_FLAGS, QDOM_THREAD_CONTROL,
        autoTypeInfo, 1, codeTypeInfo, QORE_PARAM_NO_ARG, "code");
#endif
}

static void nats_module_ns_init(QoreNamespace* rns, QoreNamespace* qns, ExceptionSink& xsink) {
    qns->addNamespace(NatsNs.copy());
}

static void nats_module_delete() {
#ifdef _QORE_HAS_FORK_CHILD_QUICK_EXIT
    qore_unregister_fork_handler(nats_fork_reg);
    nats_fork_reg = nullptr;
#endif
    // in a child process created with fork(), the threads of the C client library, if the parent opened it, exist
    // only in the parent: closing the library would join them and wait forever, so it is left as it is, and its
    // threads, if the child opened it, end with the process (see NatsCreatorProcess)
    if (getpid() != nats_module_pid) {
        return;
    }
    // nats.c library cleanup; wait for async operations to finish
    nats_CloseAndWait(5000);
}
