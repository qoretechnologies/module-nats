# -*- mode: qore; indent-tabs-mode: nil -*-
#! @file NatsJSStreamUpdateDataProvider.qc JetStream stream update data provider

/*  NatsJSStreamUpdateDataProvider.qc Copyright 2026 Qore Technologies, s.r.o.

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

#! JetStream stream update data provider
/** @since NatsJetStreamDataProvider 1.0
*/
public class NatsJSStreamUpdateDataProvider inherits AbstractDataProvider {
    public {
        static NatsJSStreamConfigDataType RequestType();
        static NatsJSStreamInfoDataType ResponseType();
    }

    private {
        JetStreamContext js;
    }

    constructor(JetStreamContext js) {
        self.js = js;
    }

    string getName() {
        return "update";
    }

    *string getDesc() {
        return "Updates the configuration of an existing JetStream stream.\n\n"
            "Modifies stream settings such as subject filters, retention policy, storage "
            "limits, and replication. The stream must already exist. Not all settings can "
            "be changed after creation (e.g. storage type). Returns the updated stream "
            "information.";
    }

    private *AbstractDataProviderType getRequestTypeImpl() {
        return NatsJSStreamConfigDataType;
    }

    private *AbstractDataProviderType getResponseTypeImpl() {
        return NatsJSStreamInfoDataType;
    }

    private auto doRequestImpl(auto req, *hash<auto> request_options) {
        return js.updateStream(<NatsStreamConfig>{
            "name": req.name,
            "subjects": req.subjects,
            "retention": req.retention,
            "max_msgs": req.max_msgs,
            "max_bytes": req.max_bytes,
            "max_age": req.max_age,
            "max_msg_size": req.max_msg_size,
            "storage": req.storage,
            "num_replicas": req.num_replicas,
            "discard": req.discard,
            "description": req.description,
        });
    }

    private hash<DataProviderInfo> getStaticInfoImpl() {
        return <DataProviderInfo>{
            "type": "NatsJSStreamUpdateDataProvider",
            "supports_request": True,
        };
    }
}

} # namespace NatsJetStreamDataProvider
