# -*- mode: qore; indent-tabs-mode: nil -*-
#! @file NatsJetStreamDataProvider.qc Main NATS JetStream data provider

/*  NatsJetStreamDataProvider.qc Copyright 2026 Qore Technologies, s.r.o.

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

#! Main NATS JetStream data provider
/** Root data provider for NATS JetStream. Children are:
    - \c streams — stream management (create, delete, info, list, names, purge, update)
    - \c consumers — consumer management (create, delete, info, list, names, pause, update)
    - \c messages — direct message access (get, get-last, delete, erase)
    - \c publish — publish with JetStream acknowledgment
    - \c subscribe — push-based JetStream subscription
    - \c pull-subscribe — pull-based JetStream subscription
    - \c kv — key-value store operations (put, get, delete, keys, history, create-bucket,
      delete-bucket, status)
    - \c account-info — account resource usage and API statistics

    @since NatsJetStreamDataProvider 1.0
*/
public class NatsJetStreamDataProvider inherits AbstractDataProvider {
    public {
        #! Provider info
        const ProviderInfo = <DataProviderInfo>{
            "type": "NatsJetStreamDataProvider",
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
                    "TLS. Default port is `4222`. The server must have JetStream enabled.",
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

        #! The JetStream context
        JetStreamContext js;
    }

    #! Creates the NATS JetStream data provider
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
        js = client.getConnection().jetStream();
    }

    #! Returns the data provider name
    string getName() {
        return "natsjetstream";
    }

    #! Returns the data provider description
    *string getDesc() {
        return "Root data provider for NATS JetStream persistent messaging.\n\n"
            "Provides access to stream management, consumer management, guaranteed "
            "publish, push and pull subscriptions, and a distributed key-value store. "
            "JetStream extends core NATS with message persistence, at-least-once delivery, "
            "and exactly-once semantics.";
    }

    #! Returns a list of child data provider names
    private *list<string> getChildProviderNamesImpl() {
        return ("streams", "consumers", "messages", "publish", "subscribe", "pull-subscribe",
            "kv", "account-info");
    }

    #! Returns the given child data provider
    private *AbstractDataProvider getChildProviderImpl(string name) {
        switch (name) {
            case "streams":
                return new NatsJSStreamsDataProvider(js);
            case "consumers":
                return new NatsJSConsumersDataProvider(js);
            case "messages":
                return new NatsJSMessagesDataProvider(js);
            case "publish":
                return new NatsJSPublishDataProvider(js);
            case "subscribe":
                return new NatsJSSubscribeEventDataProvider(js, client);
            case "pull-subscribe":
                return new NatsJSPullSubscribeEventDataProvider(js, client);
            case "kv":
                return new NatsJSKVDataProvider(js);
            case "account-info":
                return new NatsJSAccountInfoDataProvider(js);
        }
    }

    #! Returns static provider info
    private hash<DataProviderInfo> getStaticInfoImpl() {
        return ProviderInfo;
    }
}

} # namespace NatsJetStreamDataProvider
