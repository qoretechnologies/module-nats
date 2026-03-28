# -*- mode: qore; indent-tabs-mode: nil -*-
#! @file NatsJSStreamInfoDataProvider.qc JetStream stream info data provider

/*  NatsJSStreamInfoDataProvider.qc Copyright 2026 Qore Technologies, s.r.o.

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

#! Request type for stream info
public class NatsJSStreamInfoRequestDataType inherits HashDataType {
    public {
        const Fields = {
            "name": {
                "type": StringType,
                "display_name": "Stream Name",
                "short_desc": "Stream name to get info for",
                "desc": "The name of the JetStream stream to retrieve information for.\n\n"
                    "The stream must already exist; an error is raised if the stream is "
                    "not found.",
                "required": True,
                "example_value": "ORDERS",
            },
        };
    }
    constructor() : HashDataType("NatsJSStreamInfoRequest") {
        addQoreFields(Fields);
    }
}

public const NatsJSStreamInfoRequestDataType = new NatsJSStreamInfoRequestDataType();

#! JetStream stream info data provider
/** @since NatsJetStreamDataProvider 1.0
*/
public class NatsJSStreamInfoDataProvider inherits AbstractDataProvider {
    public {
        static NatsJSStreamInfoRequestDataType RequestType();
        static NatsJSStreamInfoDataType ResponseType();
    }

    private {
        JetStreamContext js;
    }

    constructor(JetStreamContext js) {
        self.js = js;
    }

    string getName() {
        return "info";
    }

    *string getDesc() {
        return "Retrieves configuration and state information for a JetStream stream.\n\n"
            "Returns the stream's configuration (subjects, retention policy, storage "
            "limits) and current state (message count, byte size, consumer count). Use "
            "this to monitor stream health, verify configuration, and check storage "
            "utilization.";
    }

    private *AbstractDataProviderType getRequestTypeImpl() {
        return NatsJSStreamInfoRequestDataType;
    }

    private *AbstractDataProviderType getResponseTypeImpl() {
        return NatsJSStreamInfoDataType;
    }

    private auto doRequestImpl(auto req, *hash<auto> request_options) {
        return js.getStreamInfo(req.name);
    }

    private hash<DataProviderInfo> getStaticInfoImpl() {
        return <DataProviderInfo>{
            "type": "NatsJSStreamInfoDataProvider",
            "supports_request": True,
        };
    }
}

} # namespace NatsJetStreamDataProvider
