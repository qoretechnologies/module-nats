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
/** @since NatsDataProvider 1.0
*/
public class NatsConnection inherits ConnectionProvider::AbstractConnectionWithInfo {
    public {
        #! Connection scheme info
        const ConnectionScheme = <ConnectionSchemeInfo>{
            "display_name": "NATS Connection",
            "short_desc": "A connection to a NATS server",
            "desc": "A connection to a NATS messaging server supporting "
                "publish/subscribe, request/reply, and queue group messaging",
            "cls": Class::forName("NatsDataProvider::NatsConnection"),
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
                    "desc": "NATS authentication token",
                    "sensitive": True,
                },
                "username": <ConnectionOptionInfo>{
                    "display_name": "Username",
                    "short_desc": "Username for authentication",
                    "type": "string",
                    "desc": "Username for NATS authentication",
                },
                "password": <ConnectionOptionInfo>{
                    "display_name": "Password",
                    "short_desc": "Password for authentication",
                    "type": "string",
                    "desc": "Password for NATS authentication",
                    "sensitive": True,
                },
            },
        };
    }

    #! Creates the NatsConnection object
    constructor(string name, string description, string url, hash<auto> attributes = {},
            hash<auto> options = {})
            : AbstractConnectionWithInfo(name, description, url, attributes, options) {
    }

    #! Creates the NatsConnection object from a config hash
    constructor(hash<auto> config, *hash<auto> attr) : AbstractConnectionWithInfo(config, attr) {
    }

    #! Returns a NatsDataProvider for this connection
    DataProvider::AbstractDataProvider getDataProvider() {
        hash<auto> dp_opts({
            "url": url,
        });
        if (self.opts.token) {
            dp_opts.token = self.opts.token;
        }
        if (self.opts.username) {
            dp_opts.username = self.opts.username;
        }
        if (self.opts.password) {
            dp_opts.password = self.opts.password;
        }
        return new NatsDataProvider(dp_opts);
    }

    #! Returns "nats"
    string getType() {
        return "nats";
    }

    #! Returns the ConnectionSchemeInfo
    private hash<ConnectionSchemeInfo> getConnectionSchemeInfoImpl() {
        return ConnectionScheme;
    }

    #! Returns the underlying connection object
    private Qore::Nats::NatsConnection getImpl(bool connect = True, *hash<auto> rtopts) {
        return new Qore::Nats::NatsConnection(url);
    }

    #! Returns a ping hash for the connection
    private hash<PingInfo> pingImpl() {
        hash<PingInfo> rv;
        date start = now_us();
        try {
            Qore::Nats::NatsConnection conn(url);
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
public class NatsTlsConnection inherits NatsConnection {
    public {
        #! Connection scheme info for TLS
        const ConnectionScheme = <ConnectionSchemeInfo>{
            "display_name": "NATS TLS Connection",
            "short_desc": "A TLS-encrypted connection to a NATS server",
            "desc": "A TLS-encrypted connection to a NATS messaging server",
            "cls": Class::forName("NatsDataProvider::NatsTlsConnection"),
            "schemes": {
                "tls": True,
            },
            "base_scheme_map": {
                "tls": "tls",
            },
            "options": NatsConnection::ConnectionScheme.options,
        };
    }

    constructor(string name, string description, string url, hash<auto> attributes = {},
            hash<auto> options = {})
            : NatsConnection(name, description, url, attributes, options) {
    }

    constructor(hash<auto> config, *hash<auto> attr) : NatsConnection(config, attr) {
    }

    string getType() {
        return "tls";
    }

    private hash<ConnectionSchemeInfo> getConnectionSchemeInfoImpl() {
        return ConnectionScheme;
    }
}

} # namespace NatsDataProvider
