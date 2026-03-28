# -*- mode: qore; indent-tabs-mode: nil -*-
#! @file NatsJSStreamNamesDataProvider.qc JetStream stream names data provider

/*  NatsJSStreamNamesDataProvider.qc Copyright 2026 Qore Technologies, s.r.o.

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

#! JetStream stream names data provider
/** @since NatsJetStreamDataProvider 1.0
*/
public class NatsJSStreamNamesDataProvider inherits AbstractDataProvider {
    public {
        static NatsJSStreamNamesResponseDataType ResponseType();
    }

    private {
        JetStreamContext js;
    }

    constructor(JetStreamContext js) {
        self.js = js;
    }

    string getName() {
        return "names";
    }

    *string getDesc() {
        return "Lists all JetStream stream names.\n\n"
            "Returns only the stream names without full configuration or state details. "
            "Use this for lightweight enumeration when you need to check which streams "
            "exist without the overhead of retrieving complete stream information.";
    }

    private *AbstractDataProviderType getRequestTypeImpl() {
        return AbstractDataProviderTypeMap."nothing";
    }

    private *AbstractDataProviderType getResponseTypeImpl() {
        return NatsJSStreamNamesResponseDataType;
    }

    private auto doRequestImpl(auto req, *hash<auto> request_options) {
        return {"names": js.listStreamNames()};
    }

    private hash<DataProviderInfo> getStaticInfoImpl() {
        return <DataProviderInfo>{
            "type": "NatsJSStreamNamesDataProvider",
            "supports_request": True,
            "display_name": "Stream Names",
            "short_desc": "List all JetStream stream names",
        };
    }
}

} # namespace NatsJetStreamDataProvider
