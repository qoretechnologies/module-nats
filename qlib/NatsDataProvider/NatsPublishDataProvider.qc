# -*- mode: qore; indent-tabs-mode: nil -*-
#! @file NatsPublishDataProvider.qc NATS publish data provider

/*  NatsPublishDataProvider.qc Copyright 2026 Qore Technologies, s.r.o.

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

#! Publish request type
public const NatsPublishRequestDataType = new HashDataType("NatsPublishRequest", {
    "subject": {
        "type": StringType,
        "desc": "The subject to publish to",
        "required": True,
    },
    "data": {
        "type": DataOrNothingType,
        "desc": "The binary message payload",
    },
    "string_data": {
        "type": StringOrNothingType,
        "desc": "The string message payload (alternative to data)",
    },
    "headers": {
        "type": AutoHashOrNothingType,
        "desc": "Optional message headers",
    },
});

#! NATS publish data provider
/** @since NatsDataProvider 1.0
*/
public class NatsPublishDataProvider inherits AbstractDataProvider {
    private {
        NatsClient client;
    }

    constructor(NatsClient client) {
        self.client = client;
    }

    string getName() {
        return "publish";
    }

    *string getDesc() {
        return "Publishes a message to a NATS subject";
    }

    private *AbstractDataProviderType getRequestTypeImpl() {
        return NatsPublishRequestDataType;
    }

    private *AbstractDataProviderType getResponseTypeImpl() {
        return AbstractDataProviderTypeMap."nothing";
    }

    private auto doRequestImpl(auto req, *hash<auto> request_options) {
        if (req.headers) {
            client.publishMsg(req.subject, req.data ?? binary(req.string_data ?? ""),
                req.headers);
        } else if (req.string_data) {
            client.publishString(req.subject, req.string_data);
        } else {
            client.publish(req.subject, req.data);
        }
        client.flush();
    }

    private hash<DataProviderInfo> getStaticInfoImpl() {
        return <DataProviderInfo>{
            "type": "NatsPublishDataProvider",
            "supports_request": True,
        };
    }
}

} # namespace NatsDataProvider
