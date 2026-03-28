# -*- mode: qore; indent-tabs-mode: nil -*-
#! @file NatsJSKVDataProvider.qc Container data provider for JetStream KV store operations

/*  NatsJSKVDataProvider.qc Copyright 2026 Qore Technologies, s.r.o.

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

#! Container data provider for JetStream Key-Value store operations
/** @since NatsJetStreamDataProvider 1.0
*/
public class NatsJSKVDataProvider inherits AbstractDataProvider {
    private {
        JetStreamContext js;
    }

    constructor(JetStreamContext js) {
        self.js = js;
    }

    string getName() {
        return "kv";
    }

    *string getDesc() {
        return "Container for JetStream Key-Value store operations.\n\n"
            "Provides access to a distributed key-value store built on JetStream streams. "
            "Supports put, get, delete, key listing, and revision history. Ideal for "
            "distributed configuration, feature flags, service discovery, and lightweight "
            "caching with built-in audit trails.";
    }

    private *list<string> getChildProviderNamesImpl() {
        return ("put", "get", "delete", "keys", "history", "create-bucket", "delete-bucket",
            "status");
    }

    private *AbstractDataProvider getChildProviderImpl(string name) {
        switch (name) {
            case "put":
                return new NatsJSKVPutDataProvider(js);
            case "get":
                return new NatsJSKVGetDataProvider(js);
            case "delete":
                return new NatsJSKVDeleteDataProvider(js);
            case "keys":
                return new NatsJSKVKeysDataProvider(js);
            case "history":
                return new NatsJSKVHistoryDataProvider(js);
            case "create-bucket":
                return new NatsJSKVCreateBucketDataProvider(js);
            case "delete-bucket":
                return new NatsJSKVDeleteBucketDataProvider(js);
            case "status":
                return new NatsJSKVStatusDataProvider(js);
        }
    }

    private hash<DataProviderInfo> getStaticInfoImpl() {
        return <DataProviderInfo>{
            "type": "NatsJSKVDataProvider",
            "supports_children": True,
            "children_can_support_apis": True,
        };
    }
}

} # namespace NatsJetStreamDataProvider
