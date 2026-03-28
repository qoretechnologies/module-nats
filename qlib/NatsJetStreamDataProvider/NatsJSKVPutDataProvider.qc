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
public class NatsJSKVPutRequestDataType inherits HashDataType {
    public {
        const Fields = {
            "bucket": {
                "type": StringType,
                "display_name": "Bucket",
                "short_desc": "KV bucket name",
                "desc": "The name of the JetStream KV bucket to store the value in.\n\n"
                    "The bucket must already exist. Use dot-separated naming conventions "
                    "for organization (e.g. `config`, `cache`, `feature-flags`).",
                "required": True,
                "example_value": "config",
            },
            "key": {
                "type": StringType,
                "display_name": "Key",
                "short_desc": "Key name to store the value under",
                "desc": "The key name to store the value under.\n\n"
                    "Keys can use dot-separated hierarchies (e.g. `app.database.host`). "
                    "If the key already exists, its value is updated and a new revision "
                    "is created.",
                "required": True,
                "example_value": "app.setting",
            },
            "data": {
                "type": DataOrNothingType,
                "display_name": "Binary Data",
                "short_desc": "Binary value to store",
                "desc": "The binary value to store; use either `data` or `string_data`.\n\n"
                    "Binary data is stored as-is without encoding. If both `data` and "
                    "`string_data` are provided, `data` takes precedence.",
            },
            "string_data": {
                "type": StringOrNothingType,
                "display_name": "String Data",
                "short_desc": "String value to store",
                "desc": "The string value to store (alternative to `data`).\n\n"
                    "Use this for configuration values, JSON, or other text content. "
                    "The string is converted to binary before storage.",
                "example_value": "{\"max_connections\": 100}",
            },
        };
    }
    constructor() : HashDataType("NatsJSKVPutRequest") {
        addQoreFields(Fields);
    }
}

public const NatsJSKVPutRequestDataType = new NatsJSKVPutRequestDataType();

#! KV put response type
public class NatsJSKVPutResponseDataType inherits HashDataType {
    public {
        const Fields = {
            "revision": {
                "type": IntType,
                "display_name": "Revision",
                "short_desc": "The revision number of the put value",
                "desc": "The new revision number assigned to the value after the put "
                    "operation.\n\n"
                    "Revision numbers are monotonically increasing across all keys in "
                    "the bucket. This value can be used for optimistic concurrency "
                    "control on subsequent updates.",
                "example_value": 1,
            },
        };
    }
    constructor() : HashDataType("NatsJSKVPutResponse") {
        addQoreFields(Fields);
    }
}

public const NatsJSKVPutResponseDataType = new NatsJSKVPutResponseDataType();

#! JetStream KV put data provider
/** @since NatsJetStreamDataProvider 1.0
*/
public class NatsJSKVPutDataProvider inherits AbstractDataProvider {
    public {
        static NatsJSKVPutRequestDataType RequestType();
        static NatsJSKVPutResponseDataType ResponseType();
    }

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
        return "Stores a value for a key in a JetStream KV bucket.\n\n"
            "Creates the key if it does not exist, or updates it with a new revision if "
            "it does. Returns the new `revision` number, which can be used for optimistic "
            "concurrency control. Ideal for distributed configuration, feature flags, and "
            "shared state management.";
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
