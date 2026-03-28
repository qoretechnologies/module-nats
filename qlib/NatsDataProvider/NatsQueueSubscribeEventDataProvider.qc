# -*- mode: qore; indent-tabs-mode: nil -*-
#! @file NatsQueueSubscribeEventDataProvider.qc NATS queue subscribe event data provider

/*  NatsQueueSubscribeEventDataProvider.qc Copyright 2026 Qore Technologies, s.r.o.

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

#! Contains all public definitions in the NatsDataProvider module
public namespace NatsDataProvider {

#! Queue subscribe request type
public class NatsQueueSubscribeRequestDataType inherits HashDataType {
    public {
        const Fields = {
            "subject": {
                "type": StringType,
                "display_name": "Subject",
                "short_desc": "NATS subject to subscribe to",
                "desc": "The NATS subject to subscribe to (supports wildcards `*` and `>`).\n\n"
                    "Use `*` to match a single token (e.g. `tasks.*`) or `>` to match one "
                    "or more tokens (e.g. `tasks.>` matches `tasks.process`, "
                    "`tasks.process.high`, etc.).",
                "required": True,
                "example_value": "tasks.>",
            },
            "queue": {
                "type": StringType,
                "display_name": "Queue Group",
                "short_desc": "Queue group name for load balancing",
                "desc": "The queue group name for load-balanced message delivery.\n\n"
                    "All subscribers sharing the same `queue` group name form a worker "
                    "pool. Each message is delivered to **exactly one** member of the group, "
                    "enabling horizontal scaling of message processing across multiple "
                    "instances without duplicate processing.",
                "required": True,
                "example_value": "workers",
            },
        };
    }
    constructor() : HashDataType("NatsQueueSubscribeRequest") {
        addQoreFields(Fields);
    }
}

#! Singleton queue subscribe request type instance
public const NatsQueueSubscribeRequestDataType = new NatsQueueSubscribeRequestDataType();

#! Observable data provider for NATS queue subscriptions
/** @since NatsDataProvider 1.0
*/
public class NatsQueueSubscribeEventDataProvider inherits AbstractDataProvider, DelayedObservable {
    private {
        NatsClient client;
        *hash<auto> observer_request;

        #! The channel subscription (created in observersReady for deterministic startup)
        NatsChannelSubscription csub;

        bool started = False;
        bool stopping = False;
        bool active = True;
        Mutex m();
    }

    constructor(NatsClient client) {
        self.client = client;
    }

    string getName() {
        return "queue-subscribe";
    }

    *string getDesc() {
        return "Observable event provider for NATS queue group subscriptions.\n\n"
            "Provides load-balanced message delivery across multiple subscriber instances. "
            "Each message matching the `subject` is delivered to **exactly one** member of "
            "the `queue` group, making this ideal for scaling out worker pools and "
            "preventing duplicate processing.";
    }

    setObserverRequest(*hash<auto> req) {
        observer_request = req;
    }

    observersReady() {
        {
            AutoLock al(m);
            if (started) {
                throw "NATS-OBSERVABLE-ERROR", "observersReady() has already been called; "
                    "create a new provider instance for a new subscription";
            }
            started = True;
        }
        # Create subscription synchronously so it's active before this method returns
        string subject = observer_request.subject ?? ">";
        string queue = observer_request.queue ?? "default";
        csub = client.channelQueueSubscribe(subject, queue);
        # Flush to ensure SUBSCRIBE reaches the server before this method returns
        client.flush();
        background queueSubscribeLoop();
    }

    stopEvents() {
        {
            AutoLock al(m);
            stopping = True;
        }
        csub.close();
    }

    bool isActive() {
        AutoLock al(m);
        return active;
    }

    private hash<DataProviderInfo> getStaticInfoImpl() {
        return <DataProviderInfo>{
            "type": "NatsQueueSubscribeEventDataProvider",
            "supports_observable": True,
        };
    }

    private hash<string, hash<DataProviderMessageInfo>> getEventTypesImpl() {
        return {
            EVENT_NATS_MESSAGE: <DataProviderMessageInfo>{
                "desc": "A message received from the NATS queue subscription",
                "type": NatsMessageDataType,
            },
            EVENT_NATS_COMPLETE: <DataProviderMessageInfo>{
                "desc": "The queue subscription completed",
                "type": NatsCompleteDataType,
            },
            EVENT_NATS_ERROR: <DataProviderMessageInfo>{
                "desc": "An error occurred on the queue subscription",
                "type": NatsErrorDataType,
            },
        };
    }

    private queueSubscribeLoop() {
        @debug("NatsQueueSubscribeEventDataProvider::queueSubscribeLoop() started");
        try {
            foreach hash<NatsMsgInfo> msg in (csub.iterator()) {
                {
                    AutoLock al(m);
                    if (stopping) {
                        break;
                    }
                }
                notifyObservers(EVENT_NATS_MESSAGE, msg);
            }

            {
                AutoLock al(m);
                active = False;
            }
            notifyObservers(EVENT_NATS_COMPLETE, {});
        } catch (hash<ExceptionInfo> ex) {
            {
                AutoLock al(m);
                active = False;
            }
            notifyObservers(EVENT_NATS_ERROR, {
                "err": ex.err,
                "desc": ex.desc,
            });
        }
    }
}

} # namespace NatsDataProvider
