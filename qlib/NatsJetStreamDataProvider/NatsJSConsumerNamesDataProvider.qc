# -*- mode: qore; indent-tabs-mode: nil -*-
#! @file NatsJSConsumerNamesDataProvider.qc JetStream consumer names data provider

/*  NatsJSConsumerNamesDataProvider.qc Copyright 2026 Qore Technologies, s.r.o.

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

#! Request type for consumer names
public class NatsJSConsumerNamesRequestDataType inherits HashDataType {
    public {
        const Fields = {
            "stream": {
                "type": StringType,
                "display_name": "Stream Name",
                "short_desc": "Stream name to list consumer names for",
                "desc": "The name of the JetStream stream to list consumer names for.\n\n"
                    "The stream must already exist; an error is raised if the stream is "
                    "not found.",
                "required": True,
                "example_value": "ORDERS",
            },
        };
    }
    constructor() : HashDataType("NatsJSConsumerNamesRequest") {
        addQoreFields(Fields);
    }
}

public const NatsJSConsumerNamesRequestDataType = new NatsJSConsumerNamesRequestDataType();

#! JetStream consumer names data provider
/** @since NatsJetStreamDataProvider 1.0
*/
public class NatsJSConsumerNamesDataProvider inherits AbstractDataProvider {
    public {
        static NatsJSConsumerNamesRequestDataType RequestType();
        static NatsJSConsumerNamesResponseDataType ResponseType();
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
        return "Lists all consumer names for a JetStream stream.\n\n"
            "Returns only the consumer names without full configuration or state details. "
            "Use this for lightweight enumeration when you need to check which consumers "
            "exist without the overhead of retrieving complete consumer information.";
    }

    private *AbstractDataProviderType getRequestTypeImpl() {
        return NatsJSConsumerNamesRequestDataType;
    }

    private *AbstractDataProviderType getResponseTypeImpl() {
        return NatsJSConsumerNamesResponseDataType;
    }

    private auto doRequestImpl(auto req, *hash<auto> request_options) {
        return {"names": js.listConsumerNames(req.stream)};
    }

    private hash<DataProviderInfo> getStaticInfoImpl() {
        return <DataProviderInfo>{
            "type": "NatsJSConsumerNamesDataProvider",
            "supports_request": True,
        };
    }
}

} # namespace NatsJetStreamDataProvider
