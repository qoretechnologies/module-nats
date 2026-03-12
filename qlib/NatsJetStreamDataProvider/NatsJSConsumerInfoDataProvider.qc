# -*- mode: qore; indent-tabs-mode: nil -*-
#! @file NatsJSConsumerInfoDataProvider.qc JetStream consumer info data provider

/*  NatsJSConsumerInfoDataProvider.qc Copyright 2026 Qore Technologies, s.r.o.

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

#! Consumer info request type
public const NatsJSConsumerInfoRequestDataType = new HashDataType("NatsJSConsumerInfoRequest", {
    "stream": {
        "type": StringType,
        "desc": "Stream name",
        "required": True,
    },
    "consumer": {
        "type": StringType,
        "desc": "Consumer name",
        "required": True,
    },
});

#! JetStream consumer info data provider
/** @since NatsJetStreamDataProvider 1.0
*/
public class NatsJSConsumerInfoDataProvider inherits AbstractDataProvider {
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
        return "Gets information about a JetStream consumer";
    }

    private *AbstractDataProviderType getRequestTypeImpl() {
        return NatsJSConsumerInfoRequestDataType;
    }

    private *AbstractDataProviderType getResponseTypeImpl() {
        return NatsJSConsumerInfoDataType;
    }

    private auto doRequestImpl(auto req, *hash<auto> request_options) {
        return js.getConsumerInfo(req.stream, req.consumer);
    }

    private hash<DataProviderInfo> getStaticInfoImpl() {
        return <DataProviderInfo>{
            "type": "NatsJSConsumerInfoDataProvider",
            "supports_request": True,
        };
    }
}

} # namespace NatsJetStreamDataProvider
