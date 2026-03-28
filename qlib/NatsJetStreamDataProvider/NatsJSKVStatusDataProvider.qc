# -*- mode: qore; indent-tabs-mode: nil -*-
#! @file NatsJSKVStatusDataProvider.qc JetStream KV bucket status data provider

/*  NatsJSKVStatusDataProvider.qc Copyright 2026 Qore Technologies, s.r.o.

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

#! Request type for KV bucket status
public class NatsJSKVStatusRequestDataType inherits HashDataType {
    public {
        const Fields = {
            "bucket": {
                "type": StringType,
                "display_name": "Bucket Name",
                "short_desc": "Bucket name to get status for",
                "desc": "The name of the JetStream KV bucket to retrieve status for.\n\n"
                    "The bucket must already exist; an error is raised if not found.",
                "required": True,
                "example_value": "config",
            },
        };
    }
    constructor() : HashDataType("NatsJSKVStatusRequest") {
        addQoreFields(Fields);
    }
}

public const NatsJSKVStatusRequestDataType = new NatsJSKVStatusRequestDataType();

#! JetStream KV bucket status data provider
/** @since NatsJetStreamDataProvider 1.0
*/
public class NatsJSKVStatusDataProvider inherits AbstractDataProvider {
    public {
        static NatsJSKVStatusRequestDataType RequestType();
        static NatsJSKVBucketStatusDataType ResponseType();
    }

    private {
        JetStreamContext js;
    }

    constructor(JetStreamContext js) {
        self.js = js;
    }

    string getName() {
        return "status";
    }

    *string getDesc() {
        return "Retrieves status information for a JetStream KV bucket.\n\n"
            "Returns the bucket's configuration and current state including key count, "
            "history depth, TTL, replica count, and total storage usage. Use this for "
            "monitoring bucket health, capacity planning, and operational dashboards.";
    }

    private *AbstractDataProviderType getRequestTypeImpl() {
        return NatsJSKVStatusRequestDataType;
    }

    private *AbstractDataProviderType getResponseTypeImpl() {
        return NatsJSKVBucketStatusDataType;
    }

    private auto doRequestImpl(auto req, *hash<auto> request_options) {
        NatsKeyValueStore kv = js.keyValue(req.bucket);
        return kv.status();
    }

    private hash<DataProviderInfo> getStaticInfoImpl() {
        return <DataProviderInfo>{
            "type": "NatsJSKVStatusDataProvider",
            "supports_request": True,
        };
    }
}

} # namespace NatsJetStreamDataProvider
