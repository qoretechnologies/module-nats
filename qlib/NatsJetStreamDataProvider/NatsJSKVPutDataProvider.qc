# -*- mode: qore; indent-tabs-mode: nil -*-
#! @file NatsJSKVPutDataProvider.qc JetStream KV put data provider

/*  NatsJSKVPutDataProvider.qc Copyright 2026 Qore Technologies, s.r.o.

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

#! KV put request type
public const NatsJSKVPutRequestDataType = new HashDataType("NatsJSKVPutRequest", {
    "bucket": {
        "type": StringType,
        "desc": "KV bucket name",
        "required": True,
    },
    "key": {
        "type": StringType,
        "desc": "Key name",
        "required": True,
    },
    "data": {
        "type": DataOrNothingType,
        "desc": "Binary value",
    },
    "string_data": {
        "type": StringOrNothingType,
        "desc": "String value (alternative to data)",
    },
});

#! KV put response type
public const NatsJSKVPutResponseDataType = new HashDataType("NatsJSKVPutResponse", {
    "revision": {
        "type": IntType,
        "desc": "The revision number of the put value",
    },
});

#! JetStream KV put data provider
/** @since NatsJetStreamDataProvider 1.0
*/
public class NatsJSKVPutDataProvider inherits AbstractDataProvider {
    private {
        JetStreamContext js;
    }

    constructor(JetStreamContext js) {
        self.js = js;
    }

    string getName() {
        return "put";
    }

    *string getDesc() {
        return "Puts a value for a key in a NATS JetStream Key-Value store";
    }

    private *AbstractDataProviderType getRequestTypeImpl() {
        return NatsJSKVPutRequestDataType;
    }

    private *AbstractDataProviderType getResponseTypeImpl() {
        return NatsJSKVPutResponseDataType;
    }

    private auto doRequestImpl(auto req, *hash<auto> request_options) {
        NatsKeyValueStore kv = js.keyValue(req.bucket);
        int revision;
        if (req.string_data) {
            revision = kv.putString(req.key, req.string_data);
        } else {
            revision = kv.put(req.key, req.data ?? binary());
        }
        return {"revision": revision};
    }

    private hash<DataProviderInfo> getStaticInfoImpl() {
        return <DataProviderInfo>{
            "type": "NatsJSKVPutDataProvider",
            "supports_request": True,
        };
    }
}

} # namespace NatsJetStreamDataProvider
