# -*- mode: qore; indent-tabs-mode: nil -*-
#! @file NatsJSConsumerDeleteDataProvider.qc JetStream consumer delete data provider

/*  NatsJSConsumerDeleteDataProvider.qc Copyright 2026 Qore Technologies, s.r.o.

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

#! Consumer delete request type
public class NatsJSConsumerDeleteRequestDataType inherits HashDataType {
    public {
        const Fields = {
            "stream": {
                "type": StringType,
                "display_name": "Stream Name",
                "short_desc": "Stream name containing the consumer",
                "desc": "The name of the JetStream stream that owns the consumer to delete.\n\n"
                    "The stream must exist and contain the specified consumer.",
                "required": True,
                "example_value": "ORDERS",
            },
            "consumer": {
                "type": StringType,
                "display_name": "Consumer Name",
                "short_desc": "Consumer name to delete",
                "desc": "The name of the consumer to delete from the stream.\n\n"
                    "**Warning**: deleting a consumer discards its tracking state, "
                    "including the last acknowledged sequence position. If the consumer is "
                    "re-created, it will start from the configured delivery policy, not "
                    "from where it left off.",
                "required": True,
                "example_value": "order-processor",
            },
        };
    }
    constructor() : HashDataType("NatsJSConsumerDeleteRequest") {
        addQoreFields(Fields);
    }
}

public const NatsJSConsumerDeleteRequestDataType = new NatsJSConsumerDeleteRequestDataType();

#! JetStream consumer delete data provider
/** @since NatsJetStreamDataProvider 1.0
*/
public class NatsJSConsumerDeleteDataProvider inherits AbstractDataProvider {
    public {
        static NatsJSConsumerDeleteRequestDataType RequestType();
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
        return "Deletes a consumer from a JetStream stream.\n\n"
            "**Warning**: this permanently removes the consumer and its tracking state. "
            "Any unacknowledged messages will not be redelivered. Active subscriptions "
            "using this consumer will be terminated.";
    }

    private *AbstractDataProviderType getRequestTypeImpl() {
        return NatsJSConsumerDeleteRequestDataType;
    }

    private *AbstractDataProviderType getResponseTypeImpl() {
        return AbstractDataProviderTypeMap."nothing";
    }

    private auto doRequestImpl(auto req, *hash<auto> request_options) {
        js.deleteConsumer(req.stream, req.consumer);
    }

    private hash<DataProviderInfo> getStaticInfoImpl() {
        return <DataProviderInfo>{
            "type": "NatsJSConsumerDeleteDataProvider",
            "supports_request": True,
        };
    }
}

} # namespace NatsJetStreamDataProvider
