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
                "desc": "The JetStream subject to subscribe to.\n\n"
                    "Supports NATS wildcard tokens: `*` matches a single token "
                    "(e.g. `orders.*` matches `orders.created`) and `>` matches one or "
                    "more tokens (e.g. `orders.>` matches `orders.created`, "
                    "`orders.us.created`, etc.).",
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
            "refusal_delay_ms": {
                "type": IntOrNothingType,
                "display_name": "Refusal Delay",
                "short_desc": "Initial redelivery delay in milliseconds of a refused message",
                "desc": "The delay in milliseconds after which the server delivers a message that an observer "
                    "refused again; it doubles with each delivery of the message up to `max_refusal_delay_ms`.  "
                    "The default is 1000",
                "example_value": 1000,
            },
            "max_refusal_delay_ms": {
                "type": IntOrNothingType,
                "display_name": "Max Refusal Delay",
                "short_desc": "Maximum redelivery delay in milliseconds of a refused message",
                "desc": "The maximum delay in milliseconds after which the server delivers a message that an "
                    "observer refused again.  The default is 60000",
                "example_value": 60000,
            },
        };
    }
    constructor() : HashDataType("NatsJSPullSubscribeRequest") {
        addQoreFields(Fields);
    }
}

public const NatsJSPullSubscribeRequestDataType = new NatsJSPullSubscribeRequestDataType();

#! Observable data provider for JetStream pull subscriptions
/** Fetches the messages of the subscription in batches.  A fetch error is logged and the provider fetches again
    after a delay; only a subscription that is no longer valid, or whose connection is closed, ends it.  The
    subscription is not unsubscribed when the provider stops, so that a durable consumer that it created is kept.

    Each message is acknowledged after every observer has accepted it, and a refused message is delivered again
    after a delay (see @ref natsjetstream_delivery); a refusal never ends the subscription.

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

        #! The pull subscription (created in observersReady for deterministic startup)
        NatsSubscription nsub;

        #! Flag: stream started
        bool started = False;

        #! Flag: stop requested
        bool stopping = False;

        #! Flag: stream active
        bool active = True;

        #! Mutex
        Mutex m();

        #! Wakes the wait after a fetch error when the provider is stopped
        Condition cond();

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
        nsub = js.pullSubscribe(subject, durable);
        background pullSubscribeLoop();
    }

    #! Stops event delivery; the pull loop ends after its current fetch, which waits for at most @ref FetchWaitMs
    stopEvents() {
        AutoLock al(m);
        stopping = True;
        cond.broadcast();
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
            "type": "NatsJSPullSubscribeEventDataProvider",
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

    #! The number of messages that one fetch requests
    const FetchBatch = 10;

    #! The longest time in milliseconds that one fetch waits, so that a stop request is seen promptly
    const FetchWaitMs = 1000;

    #! The time in milliseconds to wait after a fetch error before fetching again
    const FetchErrorDelayMs = 1000;

    private pullSubscribeLoop() {
        @debug("NatsJSPullSubscribeEventDataProvider::pullSubscribeLoop() started");
        try {
            # the messages of a pull subscription are fetched: nextMsg() does not request them from the consumer;
            # the subscription is not unsubscribed when the loop ends, which would delete a durable consumer that
            # it created
            # consecutive refusals; gives the redelivery delay of a message without a delivery count
            int refusals = 0;
            while (True) {
                {
                    AutoLock al(m);
                    if (stopping) {
                        break;
                    }
                }
                list<hash<NatsMsgInfo>> msgs;
                try {
                    msgs = nsub.fetch(FetchBatch, milliseconds(FetchWaitMs));
                } catch (hash<ExceptionInfo> ex) {
                    {
                        AutoLock al(m);
                        if (stopping) {
                            break;
                        }
                    }
                    # a subscription that is no longer valid, or whose connection is closed, cannot fetch again
                    if (ex.err == "NATS-SUBSCRIBE-ERROR"
                        && ex.desc =~ /not valid|Invalid Subscription|Connection Closed/) {
                        rethrow;
                    }
                    Logger::LoggerWrapper::error("NatsJSPullSubscribeEventDataProvider: fetch error; fetching again "
                        "in %d ms: %s: %s", FetchErrorDelayMs, ex.err, ex.desc);
                    waitBeforeRetry(FetchErrorDelayMs);
                    continue;
                }
                foreach hash<NatsMsgInfo> msg in (msgs) {
                    {
                        AutoLock al(m);
                        if (stopping) {
                            # the messages fetched but not delivered are delivered again at once
                            map settle(\NatsJSDelivery::nak(), $1, 0), msgs[$#..];
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
                            "%d ms: %s: %s", "NatsJSPullSubscribeEventDataProvider", msg.subject, delay_ms,
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

    #! Waits for the given time; returns early when the provider is stopped
    private waitBeforeRetry(int delay_ms) {
        date end = now_us() + milliseconds(delay_ms);
        AutoLock al(m);
        while (!stopping) {
            int remaining_ms = (end - now_us()).durationMilliseconds();
            if (remaining_ms <= 0) {
                break;
            }
            cond.wait(m, remaining_ms);
        }
    }
}

} # namespace NatsJetStreamDataProvider
