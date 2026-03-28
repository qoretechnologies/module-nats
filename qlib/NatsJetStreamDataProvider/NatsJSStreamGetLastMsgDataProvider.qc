# -*- mode: qore; indent-tabs-mode: nil -*-
#! @file NatsJSStreamGetLastMsgDataProvider.qc JetStream stream get last message data provider

/*  NatsJSStreamGetLastMsgDataProvider.qc Copyright 2026 Qore Technologies, s.r.o.

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

#! JetStream stream get last message data provider
/** @since NatsJetStreamDataProvider 1.0
*/
public class NatsJSStreamGetLastMsgDataProvider inherits AbstractDataProvider {
    public {
        static NatsJSStreamMsgBySubjectRequestDataType RequestType();
        static NatsJSMessageDataType ResponseType();
    }

    private {
        JetStreamContext js;
    }

    constructor(JetStreamContext js) {
        self.js = js;
    }

    string getName() {
        return "get-last";
    }

    *string getDesc() {
        return "Retrieves the last message for a subject from a JetStream stream.\n\n"
            "Returns the most recently published message on the specified subject within "
            "the stream. Returns `NOTHING` if no message exists for the subject. Use this "
            "for getting the latest state, checking the most recent event, or implementing "
            "last-value-cache patterns.";
    }

    private *AbstractDataProviderType getRequestTypeImpl() {
        return NatsJSStreamMsgBySubjectRequestDataType;
    }

    private *AbstractDataProviderType getResponseTypeImpl() {
        return NatsJSMessageDataType;
    }

    private auto doRequestImpl(auto req, *hash<auto> request_options) {
        return js.getLastMsg(req.stream, req.subject);
    }

    private hash<DataProviderInfo> getStaticInfoImpl() {
        return <DataProviderInfo>{
            "type": "NatsJSStreamGetLastMsgDataProvider",
            "supports_request": True,
        };
    }
}

} # namespace NatsJetStreamDataProvider
