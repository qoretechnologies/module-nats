# -*- mode: qore; indent-tabs-mode: nil -*-
#! @file NatsJSKVCreateBucketDataProvider.qc JetStream KV create bucket data provider

/*  NatsJSKVCreateBucketDataProvider.qc Copyright 2026 Qore Technologies, s.r.o.

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

#! JetStream KV create bucket data provider
/** @since NatsJetStreamDataProvider 1.0
*/
public class NatsJSKVCreateBucketDataProvider inherits AbstractDataProvider {
    public {
        static NatsJSKVConfigRequestDataType RequestType();
        static NatsJSKVBucketStatusDataType ResponseType();
    }

    private {
        JetStreamContext js;
    }

    constructor(JetStreamContext js) {
        self.js = js;
    }

    string getName() {
        return "create-bucket";
    }

    *string getDesc() {
        return "Creates a new JetStream Key-Value store bucket.\n\n"
            "Creates a new KV bucket with the specified configuration including history "
            "depth, TTL, storage type, and replication. Returns the initial bucket status. "
            "Use KV buckets for distributed configuration, feature flags, service discovery, "
            "and lightweight caching.";
    }

    private *AbstractDataProviderType getRequestTypeImpl() {
        return NatsJSKVConfigRequestDataType;
    }

    private *AbstractDataProviderType getResponseTypeImpl() {
        return NatsJSKVBucketStatusDataType;
    }

    private auto doRequestImpl(auto req, *hash<auto> request_options) {
        NatsKeyValueStore kv = js.createKeyValue(<NatsKVConfig>{
            "bucket": req.bucket,
            "description": req.description,
            "max_value_size": req.max_value_size,
            "history": req.history,
            "ttl": req.ttl,
            "max_bytes": req.max_bytes,
            "storage": req.storage,
            "num_replicas": req.num_replicas,
        });
        return kv.status();
    }

    private hash<DataProviderInfo> getStaticInfoImpl() {
        return <DataProviderInfo>{
            "type": "NatsJSKVCreateBucketDataProvider",
            "supports_request": True,
        };
    }
}

} # namespace NatsJetStreamDataProvider
