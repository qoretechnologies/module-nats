# -*- mode: qore; indent-tabs-mode: nil -*-
#! @file NatsJSKVDeleteBucketDataProvider.qc JetStream KV delete bucket data provider

/*  NatsJSKVDeleteBucketDataProvider.qc Copyright 2026 Qore Technologies, s.r.o.

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

#! Request type for KV delete bucket
public class NatsJSKVDeleteBucketRequestDataType inherits HashDataType {
    public {
        const Fields = {
            "bucket": {
                "type": StringType,
                "display_name": "Bucket Name",
                "short_desc": "Bucket name to delete",
                "desc": "The name of the JetStream KV bucket to delete.\n\n"
                    "**Warning**: this permanently removes the bucket, all its keys, and "
                    "all revision history. This operation cannot be undone.",
                "required": True,
                "example_value": "config",
            },
        };
    }
    constructor() : HashDataType("NatsJSKVDeleteBucketRequest") {
        addQoreFields(Fields);
    }
}

public const NatsJSKVDeleteBucketRequestDataType = new NatsJSKVDeleteBucketRequestDataType();

#! JetStream KV delete bucket data provider
/** @since NatsJetStreamDataProvider 1.0
*/
public class NatsJSKVDeleteBucketDataProvider inherits AbstractDataProvider {
    public {
        static NatsJSKVDeleteBucketRequestDataType RequestType();
    }

    private {
        JetStreamContext js;
    }

    constructor(JetStreamContext js) {
        self.js = js;
    }

    string getName() {
        return "delete-bucket";
    }

    *string getDesc() {
        return "Permanently deletes a JetStream Key-Value store bucket.\n\n"
            "**Warning**: this is a destructive operation that removes the bucket, all "
            "stored keys, and all revision history. This action cannot be undone. Consider "
            "backing up important data before deleting a bucket.";
    }

    private *AbstractDataProviderType getRequestTypeImpl() {
        return NatsJSKVDeleteBucketRequestDataType;
    }

    private *AbstractDataProviderType getResponseTypeImpl() {
        return AbstractDataProviderTypeMap."nothing";
    }

    private auto doRequestImpl(auto req, *hash<auto> request_options) {
        js.deleteKeyValue(req.bucket);
    }

    private hash<DataProviderInfo> getStaticInfoImpl() {
        return <DataProviderInfo>{
            "type": "NatsJSKVDeleteBucketDataProvider",
            "supports_request": True,
            "display_name": "Delete Bucket",
            "short_desc": "Permanently delete a Key-Value store bucket",
        };
    }
}

} # namespace NatsJetStreamDataProvider
