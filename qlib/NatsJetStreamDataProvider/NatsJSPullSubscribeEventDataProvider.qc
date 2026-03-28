# -*- mode: qore; indent-tabs-mode: nil -*-
#! @file NatsJSPullSubscribeEventDataProvider.qc JetStream pull subscribe event data provider

/*  NatsJSPullSubscribeEventDataProvider.qc Copyright 2026 Qore Technologies, s.r.o.

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

#! Contains all public definitions in the NatsJetStreamDataProvider module
public namespace NatsJetStreamDataProvider {

#! Pull subscribe request type for JetStream pull subscriptions
public class NatsJSPullSubscribeRequestDataType inherits HashDataType {
    public {
        const Fields = {
            "subject": {
                "type": StringType,
                "display_name": "Subject",
                "short_desc": "JetStream subject to subscribe to",
                "desc": "The JetStream subject to subscribe to",
                "required": True,
                "example_value": "orders.>",
            },
            "durable": {
                "type": StringType,
                "display_name": "Durable Name",
                "short_desc": "Durable consumer name for pull subscription",
                "desc": "The durable consumer name that persists the subscription state.\n\n"
                    "Durable consumers survive client disconnects and server restarts. "
                    "When the client reconnects with the same `durable` name, message "
                    "delivery resumes from the last acknowledged position. The consumer "
                    "must already exist on the stream or will be created automatically.",
                "required": True,
                "example_value": "order-processor",
            },
        };
    }
    constructor() : HashDataType("NatsJSPullSubscribeRequest") {
        addQoreFields(Fields);
    }
}

public const NatsJSPullSubscribeRequestDataType = new NatsJSPullSubscribeRequestDataType();

#! Observable data provider for JetStream pull subscriptions
/** Uses NatsChannelSubscription internally for message delivery.

    Events raised:
    - @ref EVENT_JS_MESSAGE — a message received on the JetStream pull subscription
    - @ref EVENT_JS_COMPLETE — the subscription completed
    - @ref EVENT_JS_ERROR — an error occurred

    @since NatsJetStreamDataProvider 1.0
*/
public class NatsJSPullSubscribeEventDataProvider inherits AbstractDataProvider, DelayedObservable {
    private {
        #! The JetStream context
        JetStreamContext js;

        #! The NATS client
        NatsClient client;

        #! Request data for the subscription
        *hash<auto> observer_request;

        #! The channel subscription (created in observersReady for deterministic startup)
        NatsChannelSubscription csub;

        #! Flag: stream started
        bool started = False;

        #! Flag: stop requested
        bool stopping = False;

        #! Flag: stream active
        bool active = True;

        #! Mutex
        Mutex m();
    }

    constructor(JetStreamContext js, NatsClient client) {
        self.js = js;
        self.client = client;
    }

    string getName() {
        return "pull-subscribe";
    }

    *string getDesc() {
        return "Observable event provider for JetStream pull-based message delivery with "
            "durable consumers.\n\n"
            "The client controls the rate of message delivery by explicitly requesting "
            "batches from the server. This provides natural backpressure and is ideal for "
            "batch processing workloads. The `durable` consumer tracks the last "
            "acknowledged position, ensuring no messages are lost across restarts.";
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
        string durable = observer_request.durable;
        NatsSubscription nsub = js.pullSubscribe(subject, durable);
        csub = new NatsChannelSubscription(nsub);
        background pullSubscribeLoop();
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
            "type": "NatsJSPullSubscribeEventDataProvider",
            "supports_observable": True,
        };
    }

    private hash<string, hash<DataProviderMessageInfo>> getEventTypesImpl() {
        return {
            EVENT_JS_MESSAGE: <DataProviderMessageInfo>{
                "desc": "A message received from the JetStream pull subscription",
                "type": NatsJSMessageDataType,
            },
            EVENT_JS_COMPLETE: <DataProviderMessageInfo>{
                "desc": "The JetStream pull subscription completed",
                "type": NatsJSCompleteDataType,
            },
            EVENT_JS_ERROR: <DataProviderMessageInfo>{
                "desc": "An error occurred on the JetStream pull subscription",
                "type": NatsJSErrorDataType,
            },
        };
    }

    private pullSubscribeLoop() {
        @debug("NatsJSPullSubscribeEventDataProvider::pullSubscribeLoop() started");
        try {
            foreach hash<NatsMsgInfo> msg in (csub.iterator()) {
                {
                    AutoLock al(m);
                    if (stopping) {
                        break;
                    }
                }
                notifyObservers(EVENT_JS_MESSAGE, msg);
            }

            {
                AutoLock al(m);
                active = False;
            }
            notifyObservers(EVENT_JS_COMPLETE, {});
        } catch (hash<ExceptionInfo> ex) {
            {
                AutoLock al(m);
                active = False;
            }
            notifyObservers(EVENT_JS_ERROR, {
                "err": ex.err,
                "desc": ex.desc,
            });
        }
    }
}

} # namespace NatsJetStreamDataProvider
