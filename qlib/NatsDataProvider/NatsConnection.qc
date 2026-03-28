# -*- mode: qore; indent-tabs-mode: nil -*-
#! @file NatsConnection.qc NATS connection provider

/*  NatsConnection.qc Copyright 2026 Qore Technologies, s.r.o.

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

#! Connection class for plain NATS connections (nats://)
/** supports the options from @ref NatsDataProvider::ConstructorOptions

    @since NatsDataProvider 1.0
*/
public class NatsConnectionProvider inherits ConnectionProvider::AbstractConnectionWithInfo {
    public {
        #! Connection scheme info
        const ConnectionScheme = <ConnectionSchemeInfo>{
            "display_name": "NATS Connection",
            "short_desc": "A connection to a NATS server",
            "desc": "A connection to a NATS messaging server for publish/subscribe, "
                "request/reply, and queue group messaging patterns.\n\n"
                "Supports plaintext (`nats://`) connections with optional token or "
                "username/password authentication.",
            "cls": Class::forName("NatsDataProvider::NatsConnectionProvider"),
            "schemes": {
                "nats": True,
            },
            "base_scheme_map": {
                "nats": "nats",
            },
            "options": {
                "token": <ConnectionOptionInfo>{
                    "display_name": "Token",
                    "short_desc": "Authentication token",
                    "type": "string",
                    "desc": "Token for NATS token-based authentication.\n\n"
                        "Mutually exclusive with `username`/`password` authentication.",
                    "sensitive": True,
                },
                "username": <ConnectionOptionInfo>{
                    "display_name": "Username",
                    "short_desc": "Username for authentication",
                    "type": "string",
                    "desc": "Username for NATS username/password authentication.\n\n"
                        "Must be used together with `password`.",
                },
                "password": <ConnectionOptionInfo>{
                    "display_name": "Password",
                    "short_desc": "Password for authentication",
                    "type": "string",
                    "desc": "Password for NATS username/password authentication.\n\n"
                        "Must be used together with `username`.",
                    "sensitive": True,
                },
            },
        };
    }

    #! Creates the NatsConnectionProvider object
    constructor(string name, string desc, string url, hash<auto> attr = {},
            hash<auto> options = {}, *LoggerInterface logger)
            : AbstractConnectionWithInfo(name, desc, url, attr, options, logger) {
    }

    #! Creates the NatsConnectionProvider object from a config hash
    /** @param config with the following keys:
        - name (required string): the connection name
        - display_name (optional string): the display name
        - short_desc (optional string): a short description in plain text
        - desc (optional string): a long description with markdown formatting
        - url (required string): the connection URL
        - opts (optional hash): connection options
        - logger (optional LoggerInterface object): logger for the connection
        @param attr optional connection attributes
        - monitor (optional bool): should the connection be monitored? Default: True
        - enabled (optional bool): is the connection enabled? Default: True
        - locked (optional bool): is the connection locked? Default: False
        - debug_data (optional bool): debug data? Default: False
        - tags (optional hash): tags for the connection; no default value

        @throw CONNECTION-OPTION-ERROR missing or invalid connection option or attribute
    */
    constructor(hash<auto> config, *hash<auto> attr) : AbstractConnectionWithInfo(config, attr) {
    }

    #! Returns the string type name for the connection object
    string getType() {
        return "nats";
    }

    #! Returns @ref True, as this connection class returns a data provider with the @ref getDataProvider() method
    bool hasDataProvider() {
        return True;
    }

    #! Returns the data provider application name this connection serves
    *string getAppName() {
        return NatsDataProvider::AppName;
    }

    #! Returns a NatsDataProvider for this connection
    AbstractDataProvider getDataProvider(*hash<auto> constructor_options) {
        return new NatsDataProvider(orig_opts + {"url": url} + constructor_options);
    }

    #! Returns the ConnectionSchemeInfo
    private hash<ConnectionSchemeInfo> getConnectionSchemeInfoImpl() {
        return ConnectionScheme;
    }

    #! Returns a hash of data provider constructor option info, if applicable
    private *hash<string, hash<DataProviderOptionInfo>> getDataProviderConstructorOptionsImpl() {
        return NatsDataProvider::ConstructorOptions;
    }

    #! Override to set child data provider capabilities once per child class
    private setChildCapabilities() {
        children_can_support_apis = True;
        children_can_support_observers = True;
    }

    #! Returns NATS connection options including auth from connection options
    private hash<NatsConnectionOptions> getNatsConnectionOptions() {
        hash<NatsConnectionOptions> conn_opts(<NatsConnectionOptions>{
            "url": url,
        });
        if (orig_opts.token) {
            conn_opts.token = orig_opts.token;
        }
        if (orig_opts.username) {
            conn_opts.username = orig_opts.username;
        }
        if (orig_opts.password) {
            conn_opts.password = orig_opts.password;
        }
        return conn_opts;
    }

    #! Returns the underlying connection object
    private Qore::Nats::NatsConnection getImpl(bool connect = True, *hash<auto> rtopts) {
        return new Qore::Nats::NatsConnection(getNatsConnectionOptions());
    }

    #! Returns a ping hash for the connection
    private hash<PingInfo> pingImpl() {
        hash<PingInfo> rv;
        date start = now_us();
        try {
            Qore::Nats::NatsConnection conn(getNatsConnectionOptions());
            on_exit conn.close();
            rv.ok = True;
            rv.time = now_us() - start;
            rv.info = "OK";
        } catch (hash<ExceptionInfo> ex) {
            rv.ok = False;
            rv.time = now_us() - start;
            rv.info = sprintf("%s: %s", ex.err, ex.desc);
        }
        return rv;
    }
}

#! Connection class for TLS NATS connections (tls://)
/** @since NatsDataProvider 1.0
*/
public class NatsTlsConnectionProvider inherits NatsConnectionProvider {
    public {
        #! Connection scheme info for TLS
        const ConnectionScheme = <ConnectionSchemeInfo>{
            "display_name": "NATS TLS Connection",
            "short_desc": "A TLS-encrypted connection to a NATS server",
            "desc": "A TLS-encrypted connection to a NATS messaging server.\n\n"
                "Provides the same messaging capabilities as the plaintext connection "
                "with transport-layer encryption for secure communication.",
            "cls": Class::forName("NatsDataProvider::NatsTlsConnectionProvider"),
            "schemes": {
                "natss": True,
            },
            "base_scheme_map": {
                "tls": "natss",
            },
            "options": NatsConnectionProvider::ConnectionScheme.options,
        };
    }

    #! Creates the NatsTlsConnectionProvider object
    constructor(string name, string desc, string url, hash<auto> attr = {},
            hash<auto> options = {}, *LoggerInterface logger)
            : NatsConnectionProvider(name, desc, url, attr, options, logger) {
    }

    #! Creates the NatsTlsConnectionProvider object from a config hash
    constructor(hash<auto> config, *hash<auto> attr) : NatsConnectionProvider(config, attr) {
    }

    #! Returns the string type name for the connection object
    string getType() {
        return "natss";
    }

    private hash<ConnectionSchemeInfo> getConnectionSchemeInfoImpl() {
        return ConnectionScheme;
    }
}

} # namespace NatsDataProvider
