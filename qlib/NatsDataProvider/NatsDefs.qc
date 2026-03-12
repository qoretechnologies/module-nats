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
    public const NatsMessageDataType = new HashDataType("NatsMessageEvent", {
        "subject": {
            "type": StringType,
            "desc": "The subject the message was published to",
        },
        "data": {
            "type": DataOrNothingType,
            "desc": "The message payload",
        },
        "reply": {
            "type": StringOrNothingType,
            "desc": "The reply subject",
        },
        "headers": {
            "type": AutoHashOrNothingType,
            "desc": "Message headers",
        },
    });

    #! NATS error event data type
    public const NatsErrorDataType = new HashDataType("NatsErrorEvent", {
        "err": {
            "type": StringType,
            "desc": "The error code",
        },
        "desc": {
            "type": StringType,
            "desc": "The error description",
        },
    });

    #! NATS complete event data type
    public const NatsCompleteDataType = new HashDataType("NatsCompleteEvent");
}
