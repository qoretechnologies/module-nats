# -*- mode: qore; indent-tabs-mode: nil -*-
#! @file NatsRequestDataProvider.qc NATS request/reply data provider

/*  NatsRequestDataProvider.qc Copyright 2026 Qore Technologies, s.r.o.

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

#! Request/reply request type
public class NatsRequestRequestDataType inherits HashDataType {
    public {
        const Fields = {
            "subject": {
                "type": StringType,
                "display_name": "Subject",
                "short_desc": "NATS subject to send the request to",
                "desc": "The NATS subject to send the request to",
                "required": True,
                "example_value": "api.users.get",
            },
            "data": {
                "type": DataOrNothingType,
                "display_name": "Binary Data",
                "short_desc": "Binary request payload",
                "desc": "The binary request payload; use either `data` or `string_data`",
            },
            "string_data": {
                "type": StringOrNothingType,
                "display_name": "String Data",
                "short_desc": "String request payload",
                "desc": "The string request payload (alternative to `data`)",
                "example_value": "{\"id\": 123}",
            },
            "timeout_ms": {
                "type": IntOrNothingType,
                "display_name": "Timeout (ms)",
                "short_desc": "Request timeout in milliseconds",
                "desc": "Timeout in milliseconds for the request/reply operation (default: `5000`)",
                "example_value": 5000,
            },
        };
    }
    constructor() : HashDataType("NatsRequestRequest") {
        addQoreFields(Fields);
    }
}

#! Singleton request type instance
public const NatsRequestRequestDataType = new NatsRequestRequestDataType();

#! NATS request/reply data provider
/** @since NatsDataProvider 1.0
*/
public class NatsRequestDataProvider inherits AbstractDataProvider {
    public {
        static NatsRequestRequestDataType RequestType();
        static NatsMessageDataType ResponseType();
    }

    private {
        NatsClient client;
    }

    constructor(NatsClient client) {
        self.client = client;
    }

    string getName() {
        return "request";
    }

    *string getDesc() {
        return "Sends a synchronous request to a NATS subject and waits for a reply.\n\n"
            "Implements the request-reply messaging pattern for microservice communication. "
            "A unique reply subject (inbox) is created automatically. The call blocks until "
            "a response is received or the `timeout_ms` expires. **Only the first reply is "
            "returned**; subsequent replies to the same inbox are discarded.";
    }

    private *AbstractDataProviderType getRequestTypeImpl() {
        return NatsRequestRequestDataType;
    }

    private *AbstractDataProviderType getResponseTypeImpl() {
        return NatsMessageDataType;
    }

    private auto doRequestImpl(auto req, *hash<auto> request_options) {
        if (!req.subject) {
            throw "INVALID-REQUEST", "missing required field 'subject'";
        }
        binary payload;
        if (req.data) {
            payload = req.data;
        } else if (req.string_data) {
            payload = binary(req.string_data);
        }
        int timeout = req.timeout_ms ?? 5000;
        return client.request(req.subject, payload, timeout);
    }

    private hash<DataProviderInfo> getStaticInfoImpl() {
        return <DataProviderInfo>{
            "type": "NatsRequestDataProvider",
            "supports_request": True,
        };
    }
}

} # namespace NatsDataProvider
