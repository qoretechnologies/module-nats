# -*- mode: qore; indent-tabs-mode: nil -*-
#! @file NatsJSStreamDeleteMsgDataProvider.qc JetStream stream delete message data provider

/*  NatsJSStreamDeleteMsgDataProvider.qc Copyright 2026 Qore Technologies, s.r.o.

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

#! JetStream stream delete message data provider
/** @since NatsJetStreamDataProvider 1.0
*/
public class NatsJSStreamDeleteMsgDataProvider inherits AbstractDataProvider {
    public {
        static NatsJSStreamMsgRequestDataType RequestType();
    }

    private {
        JetStreamContext js;
    }

    constructor(JetStreamContext js) {
        self.js = js;
    }

    string getName() {
        return "delete";
    }

    *string getDesc() {
        return "Deletes a message from a JetStream stream by sequence number.\n\n"
            "Removes the message at the specified sequence number from the stream. The "
            "message data may still be recoverable from storage. For guaranteed data "
            "removal, use the `erase` action instead. Use this for selective message "
            "cleanup and retention policy enforcement.";
    }

    private *AbstractDataProviderType getRequestTypeImpl() {
        return NatsJSStreamMsgRequestDataType;
    }

    private *AbstractDataProviderType getResponseTypeImpl() {
        return AbstractDataProviderTypeMap."nothing";
    }

    private auto doRequestImpl(auto req, *hash<auto> request_options) {
        js.deleteMsg(req.stream, req.seq);
    }

    private hash<DataProviderInfo> getStaticInfoImpl() {
        return <DataProviderInfo>{
            "type": "NatsJSStreamDeleteMsgDataProvider",
            "supports_request": True,
        };
    }
}

} # namespace NatsJetStreamDataProvider
