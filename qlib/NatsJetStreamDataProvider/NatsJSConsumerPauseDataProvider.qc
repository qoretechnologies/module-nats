# -*- mode: qore; indent-tabs-mode: nil -*-
#! @file NatsJSConsumerPauseDataProvider.qc JetStream consumer pause data provider

/*  NatsJSConsumerPauseDataProvider.qc Copyright 2026 Qore Technologies, s.r.o.

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

#! Request type for consumer pause
public class NatsJSConsumerPauseRequestDataType inherits HashDataType {
    public {
        const Fields = {
            "stream": {
                "type": StringType,
                "display_name": "Stream Name",
                "short_desc": "Stream name containing the consumer",
                "desc": "The name of the JetStream stream that the consumer belongs to.\n\n"
                    "The stream must already exist; an error is raised if the stream is "
                    "not found.",
                "required": True,
                "example_value": "ORDERS",
            },
            "consumer": {
                "type": StringType,
                "display_name": "Consumer Name",
                "short_desc": "Consumer name to pause",
                "desc": "The name of the consumer to pause.\n\n"
                    "The consumer must already exist on the specified stream; an error is "
                    "raised if not found.",
                "required": True,
                "example_value": "order-processor",
            },
            "pause_until": {
                "type": DateType,
                "display_name": "Pause Until",
                "short_desc": "When the consumer should automatically resume",
                "desc": "The date/time until which the consumer should be paused.\n\n"
                    "The consumer will stop receiving messages until this time is reached, "
                    "at which point it will automatically resume delivery. Use this for "
                    "scheduled maintenance windows or rate limiting.",
                "required": True,
            },
        };
    }
    constructor() : HashDataType("NatsJSConsumerPauseRequest") {
        addQoreFields(Fields);
    }
}

public const NatsJSConsumerPauseRequestDataType = new NatsJSConsumerPauseRequestDataType();

#! JetStream consumer pause data provider
/** @since NatsJetStreamDataProvider 1.0
*/
public class NatsJSConsumerPauseDataProvider inherits AbstractDataProvider {
    public {
        static NatsJSConsumerPauseRequestDataType RequestType();
        static NatsJSConsumerPauseResponseDataType ResponseType();
    }

    private {
        JetStreamContext js;
    }

    constructor(JetStreamContext js) {
        self.js = js;
    }

    string getName() {
        return "pause";
    }

    *string getDesc() {
        return "Pauses a JetStream consumer until a specified time.\n\n"
            "A paused consumer stops receiving messages until the `pause_until` time is "
            "reached, at which point delivery automatically resumes. Use this for "
            "scheduled maintenance windows, rate limiting, or temporarily halting "
            "processing without deleting the consumer.";
    }

    private *AbstractDataProviderType getRequestTypeImpl() {
        return NatsJSConsumerPauseRequestDataType;
    }

    private *AbstractDataProviderType getResponseTypeImpl() {
        return NatsJSConsumerPauseResponseDataType;
    }

    private auto doRequestImpl(auto req, *hash<auto> request_options) {
        return js.pauseConsumer(req.stream, req.consumer, req.pause_until);
    }

    private hash<DataProviderInfo> getStaticInfoImpl() {
        return <DataProviderInfo>{
            "type": "NatsJSConsumerPauseDataProvider",
            "supports_request": True,
            "display_name": "Pause Consumer",
            "short_desc": "Pause a JetStream consumer until a specified time",
        };
    }
}

} # namespace NatsJetStreamDataProvider
