# -*- mode: qore; indent-tabs-mode: nil -*-
#! @file NatsJSPublishDataProvider.qc JetStream publish data provider

/*  NatsJSPublishDataProvider.qc Copyright 2026 Qore Technologies, s.r.o.

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

#! JetStream publish request type
public class NatsJSPublishRequestDataType inherits HashDataType {
    public {
        const Fields = {
            "subject": {
                "type": StringType,
                "display_name": "Subject",
                "short_desc": "JetStream subject to publish to",
                "desc": "The JetStream subject to publish the message to.\n\n"
                    "The subject must match at least one stream's subject filter for the "
                    "message to be persisted. If no stream captures the subject, the "
                    "publish will fail with a \"no responders\" error.",
                "required": True,
                "example_value": "orders.created",
            },
            "data": {
                "type": DataOrNothingType,
                "display_name": "Binary Data",
                "short_desc": "Binary message payload",
                "desc": "The binary message payload; use either `data` or `string_data`.\n\n"
                    "Binary data is stored as-is in the stream without encoding.",
            },
            "string_data": {
                "type": StringOrNothingType,
                "display_name": "String Data",
                "short_desc": "String message payload",
                "desc": "The string message payload (alternative to `data`).\n\n"
                    "The string is converted to binary before publishing. Use this for "
                    "JSON, XML, or other text-based payloads.",
                "example_value": "{\"order_id\": 12345, \"status\": \"created\"}",
            },
        };
    }
    constructor() : HashDataType("NatsJSPublishRequest") {
        addQoreFields(Fields);
    }
}

public const NatsJSPublishRequestDataType = new NatsJSPublishRequestDataType();

#! JetStream publish data provider
/** @since NatsJetStreamDataProvider 1.0
*/
public class NatsJSPublishDataProvider inherits AbstractDataProvider {
    public {
        static NatsJSPublishRequestDataType RequestType();
        static NatsJSPubAckDataType ResponseType();
    }

    private {
        JetStreamContext js;
    }

    constructor(JetStreamContext js) {
        self.js = js;
    }

    string getName() {
        return "publish";
    }

    *string getDesc() {
        return "Publishes a message to a JetStream subject with guaranteed delivery.\n\n"
            "Unlike basic NATS publish, JetStream publish persists the message to a stream "
            "and returns a publish acknowledgment (`pub_ack`) confirming storage. The "
            "acknowledgment includes the `stream` name and `sequence` number assigned to "
            "the message. If the message cannot be stored, an error is raised.";
    }

    private *AbstractDataProviderType getRequestTypeImpl() {
        return NatsJSPublishRequestDataType;
    }

    private *AbstractDataProviderType getResponseTypeImpl() {
        return NatsJSPubAckDataType;
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
        return js.publish(req.subject, payload);
    }

    private hash<DataProviderInfo> getStaticInfoImpl() {
        return <DataProviderInfo>{
            "type": "NatsJSPublishDataProvider",
            "supports_request": True,
        };
    }
}

} # namespace NatsJetStreamDataProvider
