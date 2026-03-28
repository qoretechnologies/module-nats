# -*- mode: qore; indent-tabs-mode: nil -*-
#! @file NatsJSMessagesDataProvider.qc Container data provider for JetStream message operations

/*  NatsJSMessagesDataProvider.qc Copyright 2026 Qore Technologies, s.r.o.

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

#! Container data provider for JetStream direct message access operations
/** @since NatsJetStreamDataProvider 1.0
*/
public class NatsJSMessagesDataProvider inherits AbstractDataProvider {
    private {
        JetStreamContext js;
    }

    constructor(JetStreamContext js) {
        self.js = js;
    }

    string getName() {
        return "messages";
    }

    *string getDesc() {
        return "Container for JetStream direct message access operations.\n\n"
            "Provides access to operations for retrieving, deleting, and securely erasing "
            "individual messages from a stream by sequence number or subject. Use these "
            "operations for message inspection, selective cleanup, and compliance with "
            "data retention policies.";
    }

    private *list<string> getChildProviderNamesImpl() {
        return ("get", "get-last", "delete", "erase");
    }

    private *AbstractDataProvider getChildProviderImpl(string name) {
        switch (name) {
            case "get":
                return new NatsJSStreamGetMsgDataProvider(js);
            case "get-last":
                return new NatsJSStreamGetLastMsgDataProvider(js);
            case "delete":
                return new NatsJSStreamDeleteMsgDataProvider(js);
            case "erase":
                return new NatsJSStreamEraseMsgDataProvider(js);
        }
    }

    private hash<DataProviderInfo> getStaticInfoImpl() {
        return <DataProviderInfo>{
            "type": "NatsJSMessagesDataProvider",
            "supports_children": True,
            "children_can_support_apis": True,
        };
    }
}

} # namespace NatsJetStreamDataProvider
