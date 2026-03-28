# -*- mode: qore; indent-tabs-mode: nil -*-
#! @file NatsJSStreamPurgeDataProvider.qc JetStream stream purge data provider

/*  NatsJSStreamPurgeDataProvider.qc Copyright 2026 Qore Technologies, s.r.o.

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

#! Request type for stream purge
public class NatsJSStreamPurgeRequestDataType inherits HashDataType {
    public {
        const Fields = {
            "name": {
                "type": StringType,
                "display_name": "Stream Name",
                "short_desc": "Stream name to purge",
                "desc": "The name of the JetStream stream to purge.\n\n"
                    "**Warning**: purging removes all messages from the stream while "
                    "preserving the stream configuration and consumers. Consumer sequence "
                    "tracking is reset. This operation cannot be undone.",
                "required": True,
                "example_value": "ORDERS",
            },
        };
    }
    constructor() : HashDataType("NatsJSStreamPurgeRequest") {
        addQoreFields(Fields);
    }
}

public const NatsJSStreamPurgeRequestDataType = new NatsJSStreamPurgeRequestDataType();

#! JetStream stream purge data provider
/** @since NatsJetStreamDataProvider 1.0
*/
public class NatsJSStreamPurgeDataProvider inherits AbstractDataProvider {
    public {
        static NatsJSStreamPurgeRequestDataType RequestType();
    }

    private {
        JetStreamContext js;
    }

    constructor(JetStreamContext js) {
        self.js = js;
    }

    string getName() {
        return "purge";
    }

    *string getDesc() {
        return "Removes all messages from a JetStream stream while preserving its "
            "configuration.\n\n"
            "**Warning**: this is a destructive operation that permanently deletes all "
            "messages in the stream. The stream configuration and consumers are preserved, "
            "but consumer sequence positions are reset. Use this for maintenance, testing, "
            "or clearing stale data.";
    }

    private *AbstractDataProviderType getRequestTypeImpl() {
        return NatsJSStreamPurgeRequestDataType;
    }

    private *AbstractDataProviderType getResponseTypeImpl() {
        return AbstractDataProviderTypeMap."nothing";
    }

    private auto doRequestImpl(auto req, *hash<auto> request_options) {
        js.purgeStream(req.name);
    }

    private hash<DataProviderInfo> getStaticInfoImpl() {
        return <DataProviderInfo>{
            "type": "NatsJSStreamPurgeDataProvider",
            "supports_request": True,
        };
    }
}

} # namespace NatsJetStreamDataProvider
