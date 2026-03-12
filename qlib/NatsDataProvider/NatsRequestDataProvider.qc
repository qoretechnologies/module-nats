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
public const NatsRequestRequestDataType = new HashDataType("NatsRequestRequest", {
    "subject": {
        "type": StringType,
        "desc": "The subject to send the request to",
        "required": True,
    },
    "data": {
        "type": DataOrNothingType,
        "desc": "The binary request payload",
    },
    "string_data": {
        "type": StringOrNothingType,
        "desc": "The string request payload (alternative to data)",
    },
    "timeout_ms": {
        "type": IntOrNothingType,
        "desc": "Timeout in milliseconds (default: 5000)",
    },
});

#! NATS request/reply data provider
/** @since NatsDataProvider 1.0
*/
public class NatsRequestDataProvider inherits AbstractDataProvider {
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
        return "Sends a NATS request and waits for a reply";
    }

    private *AbstractDataProviderType getRequestTypeImpl() {
        return NatsRequestRequestDataType;
    }

    private *AbstractDataProviderType getResponseTypeImpl() {
        return NatsMessageDataType;
    }

    private auto doRequestImpl(auto req, *hash<auto> request_options) {
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
