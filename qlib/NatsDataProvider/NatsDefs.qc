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
                    "desc": "The NATS subject the message was published to, using "
                        "dot-separated hierarchical naming (e.g., "
                        "`orders.created`, `events.user.login`).\n\n"
                        "Use wildcard subscriptions with `*` (single token) or "
                        "`>` (multiple tokens) to match multiple subjects.",
                    "example_value": "orders.created",
                },
                "data": {
                    "type": DataOrNothingType,
                    "display_name": "Data",
                    "short_desc": "The message payload",
                    "desc": "The binary message payload containing the body of "
                        "the NATS message.\n\n"
                        "This field is `NOTHING` if the message was published "
                        "without a payload. **Applications must handle the "
                        "deserialization** of the binary data according to the "
                        "expected format (e.g., JSON, Protobuf, plain text).",
                },
                "reply": {
                    "type": StringOrNothingType,
                    "display_name": "Reply Subject",
                    "short_desc": "The reply subject for request-reply messaging",
                    "desc": "The reply-to subject used for NATS request-reply "
                        "patterns. This is an auto-generated inbox address "
                        "(e.g., `_INBOX.abc123`) set by the requesting "
                        "client.\n\n"
                        "When present, the subscriber should publish a "
                        "response to this subject to complete the "
                        "request-reply exchange. This field is `NOTHING` for "
                        "fire-and-forget messages.",
                    "example_value": "_INBOX.abc123",
                },
                "headers": {
                    "type": AutoHashOrNothingType,
                    "display_name": "Headers",
                    "short_desc": "Optional message headers as key-value pairs",
                    "desc": "Optional NATS message headers providing "
                        "out-of-band metadata as key-value pairs.\n\n"
                        "Headers can carry routing hints, content types, "
                        "tracing identifiers, or application-specific metadata "
                        "without modifying the message payload. This field is "
                        "`NOTHING` when no headers were set by the publisher.",
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
                    "short_desc": "The error code identifying the type of failure",
                    "desc": "The NATS error code string identifying the type of "
                        "failure that occurred (e.g., "
                        "`NATS-CONNECTION-ERROR`).\n\n"
                        "Use this field to implement error-specific handling "
                        "logic, such as reconnection strategies or alerting "
                        "based on the error category.",
                    "example_value": "NATS-CONNECTION-ERROR",
                },
                "desc": {
                    "type": StringType,
                    "display_name": "Description",
                    "short_desc": "A human-readable description of the error",
                    "desc": "A human-readable description providing details "
                        "about the error condition.\n\n"
                        "This message is suitable for logging and diagnostic "
                        "purposes. **Do not parse this string "
                        "programmatically**; use the `err` field for "
                        "error-type identification instead.",
                    "example_value": "Connection to NATS server lost",
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
