# -*- mode: qore; indent-tabs-mode: nil -*-
#! @file NatsJSConsumersDataProvider.qc Container data provider for JetStream consumer operations

/*  NatsJSConsumersDataProvider.qc Copyright 2026 Qore Technologies, s.r.o.

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

#! Container data provider for JetStream consumer management operations
/** @since NatsJetStreamDataProvider 1.0
*/
public class NatsJSConsumersDataProvider inherits AbstractDataProvider {
    private {
        JetStreamContext js;
    }

    constructor(JetStreamContext js) {
        self.js = js;
    }

    string getName() {
        return "consumers";
    }

    *string getDesc() {
        return "Container for JetStream consumer management operations";
    }

    private *list<string> getChildProviderNamesImpl() {
        return ("create", "delete", "info");
    }

    private *AbstractDataProvider getChildProviderImpl(string name) {
        switch (name) {
            case "create":
                return new NatsJSConsumerCreateDataProvider(js);
            case "delete":
                return new NatsJSConsumerDeleteDataProvider(js);
            case "info":
                return new NatsJSConsumerInfoDataProvider(js);
        }
    }

    private hash<DataProviderInfo> getStaticInfoImpl() {
        return <DataProviderInfo>{
            "type": "NatsJSConsumersDataProvider",
            "supports_children": True,
            "children_can_support_apis": True,
        };
    }
}

} # namespace NatsJetStreamDataProvider
