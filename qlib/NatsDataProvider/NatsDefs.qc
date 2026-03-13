# -*- mode: qore; indent-tabs-mode: nil -*-
#! @file NatsDefs.qc NATS data provider constants and types

/*  NatsDefs.qc Copyright 2026 Qore Technologies, s.r.o.

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
    #! Event ID for a received NATS message
    public const EVENT_NATS_MESSAGE = "nats-message";

    #! Event ID for a NATS error
    public const EVENT_NATS_ERROR = "nats-error";

    #! Event ID for subscription completion
    public const EVENT_NATS_COMPLETE = "nats-complete";

    #! NATS message event data type
    public class NatsMessageDataType inherits HashDataType {
        public {
            const Fields = {
                "subject": {
                    "type": StringType,
                    "display_name": "Subject",
                    "short_desc": "The subject the message was published to",
                    "desc": "The subject the message was published to",
                },
                "data": {
                    "type": DataOrNothingType,
                    "display_name": "Data",
                    "short_desc": "The message payload",
                    "desc": "The binary message payload",
                },
                "reply": {
                    "type": StringOrNothingType,
                    "display_name": "Reply Subject",
                    "short_desc": "The reply subject",
                    "desc": "The reply subject, if this is a request message",
                },
                "headers": {
                    "type": AutoHashOrNothingType,
                    "display_name": "Headers",
                    "short_desc": "Message headers",
                    "desc": "Optional message headers as key-value pairs",
                },
            };
        }
        constructor() : HashDataType("NatsMessageEvent") {
            addQoreFields(Fields);
        }
    }

    #! Singleton NATS message event data type instance
    public const NatsMessageDataType = new NatsMessageDataType();

    #! NATS error event data type
    public class NatsErrorDataType inherits HashDataType {
        public {
            const Fields = {
                "err": {
                    "type": StringType,
                    "display_name": "Error Code",
                    "short_desc": "The error code",
                    "desc": "The error code",
                },
                "desc": {
                    "type": StringType,
                    "display_name": "Description",
                    "short_desc": "The error description",
                    "desc": "The error description",
                },
            };
        }
        constructor() : HashDataType("NatsErrorEvent") {
            addQoreFields(Fields);
        }
    }

    #! Singleton NATS error event data type instance
    public const NatsErrorDataType = new NatsErrorDataType();

    #! NATS complete event data type
    public const NatsCompleteDataType = new HashDataType("NatsCompleteEvent");
}
