# -*- mode: qore; indent-tabs-mode: nil -*-
#! @file NatsSubscribeEventDataProvider.qc NATS subscribe event data provider

/*  NatsSubscribeEventDataProvider.qc Copyright 2026 Qore Technologies, s.r.o.

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

#! Subscribe request type
public class NatsSubscribeRequestDataType inherits HashDataType {
    public {
        const Fields = {
            "subject": {
                "type": StringType,
                "display_name": "Subject",
                "short_desc": "NATS subject to subscribe to",
                "desc": "The NATS subject to subscribe to (supports wildcards `*` and `>`)",
                "required": True,
                "example_value": "events.>",
            },
        };
    }
    constructor() : HashDataType("NatsSubscribeRequest") {
        addQoreFields(Fields);
    }
}

#! Singleton subscribe request type instance
public const NatsSubscribeRequestDataType = new NatsSubscribeRequestDataType();

#! Observable data provider for NATS subscriptions
/** Uses NatsChannelSubscription internally for message delivery.

    Events raised:
    - @ref EVENT_NATS_MESSAGE — a message received on the subscription
    - @ref EVENT_NATS_COMPLETE — the subscription completed
    - @ref EVENT_NATS_ERROR — an error occurred

    @since NatsDataProvider 1.0
*/
public class NatsSubscribeEventDataProvider inherits AbstractDataProvider, DelayedObservable {
    private {
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

    constructor(NatsClient client) {
        self.client = client;
    }

    string getName() {
        return "subscribe";
    }

    *string getDesc() {
        return "Observable event provider for NATS subscriptions";
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
        csub = client.channelSubscribe(subject);
        # Flush to ensure SUBSCRIBE reaches the server before this method returns
        client.flush();
        background subscribeLoop();
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
            "type": "NatsSubscribeEventDataProvider",
            "supports_observable": True,
        };
    }

    private hash<string, hash<DataProviderMessageInfo>> getEventTypesImpl() {
        return {
            EVENT_NATS_MESSAGE: <DataProviderMessageInfo>{
                "desc": "A message received from the NATS subscription",
                "type": NatsMessageDataType,
            },
            EVENT_NATS_COMPLETE: <DataProviderMessageInfo>{
                "desc": "The NATS subscription completed",
                "type": NatsCompleteDataType,
            },
            EVENT_NATS_ERROR: <DataProviderMessageInfo>{
                "desc": "An error occurred on the NATS subscription",
                "type": NatsErrorDataType,
            },
        };
    }

    private subscribeLoop() {
        @debug("NatsSubscribeEventDataProvider::subscribeLoop() started");
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
