# -*- mode: qore; indent-tabs-mode: nil -*-
#! @file NatsJSKVKeysDataProvider.qc JetStream KV keys data provider

/*  NatsJSKVKeysDataProvider.qc Copyright 2026 Qore Technologies, s.r.o.

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

#! KV keys request type
public const NatsJSKVKeysRequestDataType = new HashDataType("NatsJSKVKeysRequest", {
    "bucket": {
        "type": StringType,
        "desc": "KV bucket name",
        "required": True,
    },
});

#! KV keys response type
public const NatsJSKVKeysResponseDataType = new HashDataType("NatsJSKVKeysResponse", {
    "keys": {
        "type": SoftListType,
        "desc": "List of key names in the bucket",
    },
});

#! JetStream KV keys data provider
/** @since NatsJetStreamDataProvider 1.0
*/
public class NatsJSKVKeysDataProvider inherits AbstractDataProvider {
    private {
        JetStreamContext js;
    }

    constructor(JetStreamContext js) {
        self.js = js;
    }

    string getName() {
        return "keys";
    }

    *string getDesc() {
        return "Lists all keys in a NATS JetStream Key-Value store bucket";
    }

    private *AbstractDataProviderType getRequestTypeImpl() {
        return NatsJSKVKeysRequestDataType;
    }

    private *AbstractDataProviderType getResponseTypeImpl() {
        return NatsJSKVKeysResponseDataType;
    }

    private auto doRequestImpl(auto req, *hash<auto> request_options) {
        NatsKeyValueStore kv = js.keyValue(req.bucket);
        return {"keys": kv.keys()};
    }

    private hash<DataProviderInfo> getStaticInfoImpl() {
        return <DataProviderInfo>{
            "type": "NatsJSKVKeysDataProvider",
            "supports_request": True,
        };
    }
}

} # namespace NatsJetStreamDataProvider
