# -*- mode: qore; indent-tabs-mode: nil -*-
#! @file NatsJSSubscribeEventDataProvider.qc JetStream subscribe event data provider

/*  NatsJSSubscribeEventDataProvider.qc Copyright 2026 Qore Technologies, s.r.o.

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

#! Subscribe request type for JetStream push subscriptions
public class NatsJSSubscribeRequestDataType inherits HashDataType {
    public {
        const Fields = {
            "subject": {
                "type": StringType,
                "display_name": "Subject",
                "short_desc": "JetStream subject to subscribe to",
                "desc": "The JetStream subject to subscribe to.\n\n"
                    "Supports NATS wildcard tokens: `*` matches a single token "
                    "(e.g. `orders.*` matches `orders.created`) and `>` matches one or "
                    "more tokens (e.g. `orders.>` matches `orders.created`, "
                    "`orders.us.created`, etc.).",
                "required": True,
                "example_value": "orders.>",
            },
            "refusal_delay_ms": {
                "type": IntOrNothingType,
                "display_name": "Refusal delay",
                "short_desc": "Initial redelivery delay in milliseconds of a refused message",
                "desc": "The delay in milliseconds after which the server delivers a message that an observer "
                    "refused again; it doubles with each delivery of the message up to `max_refusal_delay_ms`.  "
                    "The default is 1000",
                "example_value": 1000,
            },
            "max_refusal_delay_ms": {
                "type": IntOrNothingType,
                "display_name": "Max refusal delay",
                "short_desc": "Maximum redelivery delay in milliseconds of a refused message",
                "desc": "The maximum delay in milliseconds after which the server delivers a message that an "
                    "observer refused again.  The default is 60000",
                "example_value": 60000,
            },
        };
    }
    constructor() : HashDataType("NatsJSSubscribeRequest") {
        addQoreFields(Fields);
    }
}

public const NatsJSSubscribeRequestDataType = new NatsJSSubscribeRequestDataType();

#! Observable data provider for JetStream push subscriptions
/** Uses NatsChannelSubscription internally for message delivery.

    Each message is acknowledged after every observer has accepted it, and a refused message is delivered again
    after a delay (see @ref natsjetstream_delivery); a refusal never ends the subscription.

    Events raised:
    - @ref EVENT_JS_MESSAGE — a message received on the JetStream subscription
    - @ref EVENT_JS_COMPLETE — the subscription completed
    - @ref EVENT_JS_ERROR — an error occurred

    @since NatsJetStreamDataProvider 1.0
*/
public class NatsJSSubscribeEventDataProvider inherits AbstractDataProvider, DelayedObservable {
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

        #! The number of messages acknowledged
        int acked_count = 0;

        #! The number of refusals
        int refused_count = 0;

        #! The number of acknowledgements that could not be sent
        int settle_error_count = 0;
    }

    constructor(JetStreamContext js, NatsClient client) {
        self.js = js;
        self.client = client;
    }

    string getName() {
        return "subscribe";
    }

    *string getDesc() {
        return "Observable event provider for JetStream push-based message delivery.\n\n"
            "Messages are pushed to the subscriber as they arrive in the stream. This is "
            "suitable for real-time event processing where low latency is important. "
            "Messages include JetStream metadata such as stream sequence numbers and "
            "timestamps. For flow-controlled batch processing, use pull-subscribe instead.";
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
        NatsSubscription nsub = js.subscribe(subject);
        csub = new NatsChannelSubscription(nsub);
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

    #! Returns delivery statistics
    /** @return a hash with the following keys:
        - \c acked: the number of messages acknowledged after the observers accepted them
        - \c refused: the number of refusals (each refused message is delivered again)
        - \c settle_errors: the number of settlements that could not be sent (the server delivers those messages
          again after the consumer's ack wait)
    */
    hash<string, int> getDeliveryStats() {
        AutoLock al(m);
        return {
            "acked": acked_count,
            "refused": refused_count,
            "settle_errors": settle_error_count,
        };
    }

    #! Settles a message
    /** @return @ref True if the settlement was sent
    */
    private bool settle(code action, hash<NatsMsgInfo> msg, *int delay_ms) {
        try {
            bool rv = exists delay_ms ? action(client, msg, delay_ms) : action(client, msg);
            if (!rv) {
                # a message without a reply subject cannot be settled
                AutoLock al(m);
                ++settle_error_count;
            }
            return rv;
        } catch (hash<ExceptionInfo> ex) {
            {
                AutoLock al(m);
                ++settle_error_count;
            }
            Logger::LoggerWrapper::error("cannot settle the message on subject %y: %s: %s", msg.subject, ex.err,
                ex.desc);
        }
        return False;
    }


    private hash<DataProviderInfo> getStaticInfoImpl() {
        return <DataProviderInfo>{
            "type": "NatsJSSubscribeEventDataProvider",
            "supports_observable": True,
            "supports_refusal_redelivery": True,
        };
    }

    private auto getExampleEventDataImpl(string event_id) {
        switch (event_id) {
            case EVENT_JS_MESSAGE:
                return {
                    "subject": "orders.created",
                    "data": <68656c6c6f>,
                    "reply": NOTHING,
                    "headers": {
                        "Content-Type": "application/json",
                    },
                };
            case EVENT_JS_ERROR:
                return {
                    "err": "10037",
                    "desc": "no message found",
                };
            case EVENT_JS_COMPLETE:
                return {};
        }
    }

    private hash<string, hash<DataProviderMessageInfo>> getEventTypesImpl() {
        return {
            EVENT_JS_MESSAGE: <DataProviderMessageInfo>{
                "desc": "A message received from the JetStream subscription",
                "type": NatsJSMessageDataType,
            },
            EVENT_JS_COMPLETE: <DataProviderMessageInfo>{
                "desc": "The JetStream subscription completed",
                "type": NatsJSCompleteDataType,
            },
            EVENT_JS_ERROR: <DataProviderMessageInfo>{
                "desc": "An error occurred on the JetStream subscription",
                "type": NatsJSErrorDataType,
            },
        };
    }

    private subscribeLoop() {
        @debug("NatsJSSubscribeEventDataProvider::subscribeLoop() started");
        try {
            # consecutive refusals; gives the redelivery delay of a message without a delivery count
            int refusals = 0;
            foreach hash<NatsMsgInfo> msg in (csub.iterator()) {
                {
                    AutoLock al(m);
                    if (stopping) {
                        break;
                    }
                }
                # see @ref natsjetstream_delivery
                try {
                    notifyObservers(EVENT_JS_MESSAGE, msg);
                } catch (hash<ExceptionInfo> ex) {
                    int delay_ms = NatsJSDelivery::getDelay(NatsJSDelivery::getDeliveryCount(msg, ++refusals),
                        observer_request);
                    {
                        AutoLock al(m);
                        ++refused_count;
                    }
                    Logger::LoggerWrapper::error("%s: message on subject %y refused; it will be delivered again in "
                        "%d ms: %s: %s", "NatsJSSubscribeEventDataProvider", msg.subject, delay_ms,
                        ex.err, ex.desc);
                    settle(\NatsJSDelivery::nak(), msg, delay_ms);
                    continue;
                }
                refusals = 0;
                if (settle(\NatsJSDelivery::ack(), msg)) {
                    AutoLock al(m);
                    ++acked_count;
                }
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
