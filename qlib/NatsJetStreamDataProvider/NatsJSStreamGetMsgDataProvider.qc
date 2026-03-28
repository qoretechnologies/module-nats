# -*- mode: qore; indent-tabs-mode: nil -*-
#! @file NatsJSStreamGetMsgDataProvider.qc JetStream stream get message data provider

/*  NatsJSStreamGetMsgDataProvider.qc Copyright 2026 Qore Technologies, s.r.o.

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

#! JetStream stream get message data provider
/** @since NatsJetStreamDataProvider 1.0
*/
public class NatsJSStreamGetMsgDataProvider inherits AbstractDataProvider {
    public {
        static NatsJSStreamMsgRequestDataType RequestType();
        static NatsJSMessageDataType ResponseType();
    }

    private {
        JetStreamContext js;
    }

    constructor(JetStreamContext js) {
        self.js = js;
    }

    string getName() {
        return "get";
    }

    *string getDesc() {
        return "Retrieves a message from a JetStream stream by sequence number.\n\n"
            "Returns the message payload, subject, headers, and reply subject for the "
            "specified sequence number. Returns `NOTHING` if no message exists at the "
            "given sequence. Use this for message inspection, debugging, and audit trail "
            "access.";
    }

    private *AbstractDataProviderType getRequestTypeImpl() {
        return NatsJSStreamMsgRequestDataType;
    }

    private *AbstractDataProviderType getResponseTypeImpl() {
        return NatsJSMessageDataType;
    }

    private auto doRequestImpl(auto req, *hash<auto> request_options) {
        return js.getMsg(req.stream, req.seq);
    }

    private hash<DataProviderInfo> getStaticInfoImpl() {
        return <DataProviderInfo>{
            "type": "NatsJSStreamGetMsgDataProvider",
            "supports_request": True,
        };
    }
}

} # namespace NatsJetStreamDataProvider
