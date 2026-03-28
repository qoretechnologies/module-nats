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
public class NatsJSKVKeysRequestDataType inherits HashDataType {
    public {
        const Fields = {
            "bucket": {
                "type": StringType,
                "display_name": "Bucket",
                "short_desc": "KV bucket name to list keys for",
                "desc": "The name of the JetStream KV bucket to list keys for.\n\n"
                    "The bucket must already exist; an error is raised if not found.",
                "required": True,
                "example_value": "config",
            },
        };
    }
    constructor() : HashDataType("NatsJSKVKeysRequest") {
        addQoreFields(Fields);
    }
}

public const NatsJSKVKeysRequestDataType = new NatsJSKVKeysRequestDataType();

#! KV keys response type
public class NatsJSKVKeysResponseDataType inherits HashDataType {
    public {
        const Fields = {
            "keys": {
                "type": SoftListType,
                "display_name": "Keys",
                "short_desc": "List of key names in the bucket",
                "desc": "List of all key names currently stored in the KV bucket.\n\n"
                    "Only keys with active values are returned; deleted or purged keys "
                    "are excluded. Keys may use dot-separated hierarchies (e.g. "
                    "`app.database.host`).",
                "example_value": ("app.setting", "app.database.host"),
            },
        };
    }
    constructor() : HashDataType("NatsJSKVKeysResponse") {
        addQoreFields(Fields);
    }
}

public const NatsJSKVKeysResponseDataType = new NatsJSKVKeysResponseDataType();

#! JetStream KV keys data provider
/** @since NatsJetStreamDataProvider 1.0
*/
public class NatsJSKVKeysDataProvider inherits AbstractDataProvider {
    public {
        static NatsJSKVKeysRequestDataType RequestType();
        static NatsJSKVKeysResponseDataType ResponseType();
    }

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
        return "Lists all active keys in a JetStream KV bucket.\n\n"
            "Returns the names of all keys that have current (non-deleted) values. "
            "Deleted or purged keys are excluded. Use this for discovering available "
            "configuration keys, enumerating feature flags, or auditing bucket contents.";
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
