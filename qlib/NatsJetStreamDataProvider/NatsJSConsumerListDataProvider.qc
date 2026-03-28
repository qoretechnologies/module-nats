# -*- mode: qore; indent-tabs-mode: nil -*-
#! @file NatsJSConsumerListDataProvider.qc JetStream consumer list data provider

/*  NatsJSConsumerListDataProvider.qc Copyright 2026 Qore Technologies, s.r.o.

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

#! Request type for consumer list
public class NatsJSConsumerListRequestDataType inherits HashDataType {
    public {
        const Fields = {
            "stream": {
                "type": StringType,
                "display_name": "Stream Name",
                "short_desc": "Stream name to list consumers for",
                "desc": "The name of the JetStream stream to list consumers for.\n\n"
                    "The stream must already exist; an error is raised if the stream is "
                    "not found.",
                "required": True,
                "example_value": "ORDERS",
            },
        };
    }
    constructor() : HashDataType("NatsJSConsumerListRequest") {
        addQoreFields(Fields);
    }
}

public const NatsJSConsumerListRequestDataType = new NatsJSConsumerListRequestDataType();

#! JetStream consumer list data provider
/** @since NatsJetStreamDataProvider 1.0
*/
public class NatsJSConsumerListDataProvider inherits AbstractDataProvider {
    public {
        static NatsJSConsumerListRequestDataType RequestType();
        static NatsJSConsumerListResponseDataType ResponseType();
    }

    private {
        JetStreamContext js;
    }

    constructor(JetStreamContext js) {
        self.js = js;
    }

    string getName() {
        return "list";
    }

    *string getDesc() {
        return "Lists all consumers for a JetStream stream with their configuration and "
            "state.\n\n"
            "Returns detailed information for each consumer including delivery policy, "
            "acknowledgment settings, pending messages, and processing progress. Use this "
            "for monitoring consumer health and diagnosing delivery issues.";
    }

    private *AbstractDataProviderType getRequestTypeImpl() {
        return NatsJSConsumerListRequestDataType;
    }

    private *AbstractDataProviderType getResponseTypeImpl() {
        return NatsJSConsumerListResponseDataType;
    }

    private auto doRequestImpl(auto req, *hash<auto> request_options) {
        return {"consumers": js.listConsumers(req.stream)};
    }

    private hash<DataProviderInfo> getStaticInfoImpl() {
        return <DataProviderInfo>{
            "type": "NatsJSConsumerListDataProvider",
            "supports_request": True,
            "display_name": "List Consumers",
            "short_desc": "List all consumers for a JetStream stream",
        };
    }
}

} # namespace NatsJetStreamDataProvider
