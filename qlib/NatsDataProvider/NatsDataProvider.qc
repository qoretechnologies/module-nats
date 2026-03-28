# -*- mode: qore; indent-tabs-mode: nil -*-
#! @file NatsDataProvider.qc Main NATS data provider

/*  NatsDataProvider.qc Copyright 2026 Qore Technologies, s.r.o.

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

#! Contains all public definitions in the NatsDataProvider module
public namespace NatsDataProvider {

#! Main NATS data provider
/** Root data provider for NATS messaging. Children are:
    - \c publish — publish messages
    - \c request — request/reply
    - \c subscribe — subscribe to messages
    - \c queue-subscribe — queue group subscription

    @since NatsDataProvider 1.0
*/
public class NatsDataProvider inherits AbstractDataProvider {
    public {
        #! Provider info
        const ProviderInfo = <DataProviderInfo>{
            "type": "NatsDataProvider",
            "supports_children": True,
            "children_can_support_apis": True,
            "children_can_support_observers": True,
            "constructor_options": ConstructorOptions,
        };

        #! Constructor options
        const ConstructorOptions = {
            "url": <DataProviderOptionInfo>{
                "display_name": "URL",
                "short_desc": "NATS server URL",
                "type": AbstractDataProviderTypeMap."string",
                "desc": "The NATS server connection URL.\n\n"
                    "Format: `nats://host:port` for plaintext or `tls://host:port` for "
                    "TLS. Default port is `4222`.",
                "required": True,
            },
            "token": <DataProviderOptionInfo>{
                "display_name": "Token",
                "short_desc": "Authentication token",
                "type": AbstractDataProviderTypeMap."string",
                "desc": "Token for NATS token-based authentication.\n\n"
                    "Mutually exclusive with `username`/`password` authentication.",
            },
            "username": <DataProviderOptionInfo>{
                "display_name": "Username",
                "short_desc": "Username for authentication",
                "type": AbstractDataProviderTypeMap."string",
                "desc": "Username for NATS username/password authentication.\n\n"
                    "Must be used together with `password`.",
            },
            "password": <DataProviderOptionInfo>{
                "display_name": "Password",
                "short_desc": "Password for authentication",
                "type": AbstractDataProviderTypeMap."string",
                "desc": "Password for NATS username/password authentication.\n\n"
                    "Must be used together with `username`.",
                "sensitive": True,
            },
        };
    }

    private {
        #! The NATS client
        NatsClient client;
    }

    #! Creates the NATS data provider
    /** @param options constructor options
    */
    constructor(*hash<auto> options) {
        checkOptions("CONSTRUCTOR-OPTION-ERROR", ConstructorOptions, options);
        hash<NatsConnectionOptions> conn_opts(<NatsConnectionOptions>{
            "url": options.url,
        });
        if (options.token) {
            conn_opts.token = options.token;
        }
        if (options.username) {
            conn_opts.username = options.username;
        }
        if (options.password) {
            conn_opts.password = options.password;
        }
        client = new NatsClient(conn_opts);
    }

    #! Returns the data provider name
    string getName() {
        return "nats";
    }

    #! Returns the data provider description
    *string getDesc() {
        return "Root data provider for NATS core messaging.\n\n"
            "Provides access to publish/subscribe, request/reply, and queue group "
            "messaging patterns over a NATS connection. For persistent messaging with "
            "streams, consumers, and key-value storage, use the JetStream data provider "
            "instead.";
    }

    #! Returns a list of child data provider names
    private *list<string> getChildProviderNamesImpl() {
        return ("publish", "request", "subscribe", "queue-subscribe");
    }

    #! Returns the given child data provider
    private *AbstractDataProvider getChildProviderImpl(string name) {
        switch (name) {
            case "publish":
                return new NatsPublishDataProvider(client);
            case "request":
                return new NatsRequestDataProvider(client);
            case "subscribe":
                return new NatsSubscribeEventDataProvider(client);
            case "queue-subscribe":
                return new NatsQueueSubscribeEventDataProvider(client);
        }
    }

    #! Returns static provider info
    private hash<DataProviderInfo> getStaticInfoImpl() {
        return ProviderInfo;
    }
}

} # namespace NatsDataProvider
