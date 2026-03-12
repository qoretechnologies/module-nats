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
public const NatsJSPublishRequestDataType = new HashDataType("NatsJSPublishRequest", {
    "subject": {
        "type": StringType,
        "desc": "Subject to publish to",
        "required": True,
    },
    "data": {
        "type": DataOrNothingType,
        "desc": "Binary message payload",
    },
    "string_data": {
        "type": StringOrNothingType,
        "desc": "String message payload (alternative to data)",
    },
});

#! JetStream publish data provider
/** @since NatsJetStreamDataProvider 1.0
*/
public class NatsJSPublishDataProvider inherits AbstractDataProvider {
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
        return "Publishes a message to a JetStream subject and returns the publish acknowledgment";
    }

    private *AbstractDataProviderType getRequestTypeImpl() {
        return NatsJSPublishRequestDataType;
    }

    private *AbstractDataProviderType getResponseTypeImpl() {
        return NatsJSPubAckDataType;
    }

    private auto doRequestImpl(auto req, *hash<auto> request_options) {
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
