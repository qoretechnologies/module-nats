# -*- mode: qore; indent-tabs-mode: nil -*-
#! @file NatsHelper.qc NATS helper utilities

/*  NatsHelper.qc Copyright 2026 Qore Technologies, s.r.o.

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

#! Contains all public definitions in the NatsUtil module
public namespace Nats {

#! Parsed NATS URL components
public hashdecl NatsUrlInfo {
    #! URL scheme ("nats" or "tls")
    string scheme = "nats";
    #! Server hostname
    string host = "localhost";
    #! Server port
    int port = 4222;
    #! Optional username for authentication
    *string username;
    #! Optional password for authentication
    *string password;
}

#! NATS helper utility class
/** Provides static methods for subject validation and URL parsing.

    @since NatsUtil 1.0
*/
public class NatsHelper {
    #! Validates a NATS subject
    /** @param subject the subject to validate
        @return True if the subject is valid

        A valid subject:
        - Is not empty
        - Does not contain spaces
        - Tokens (separated by '.') are not empty
        - Wildcards '*' and '>' are valid only as complete tokens
        - '>' is only valid as the last token
    */
    static bool isValidSubject(string subject) {
        if (!subject.val()) {
            return False;
        }
        if (subject =~ /\s/) {
            return False;
        }
        list<string> tokens = subject.split(".");
        for (int i = 0; i < tokens.size(); ++i) {
            string tok = tokens[i];
            if (!tok.val()) {
                return False;
            }
            if (tok == ">") {
                # '>' must be the last token
                if (i != tokens.size() - 1) {
                    return False;
                }
            } else if (tok == "*") {
                # '*' is valid as a complete token
            } else if (tok =~ /[*>]/) {
                # '*' or '>' embedded in other characters is invalid
                return False;
            }
        }
        return True;
    }

    #! Parses a NATS URL into components
    /** @param url the NATS URL
        @return a hash<NatsUrlInfo> with parsed URL components

        Supports nats://, tls://, and bare host:port formats.
    */
    static hash<NatsUrlInfo> parseUrl(string url) {
        hash<NatsUrlInfo> result();

        string remaining = url;

        # Parse scheme
        if (remaining =~ /^(nats|tls):\/\//) {
            result.scheme = (remaining =~ x/^(nats|tls)/)[0];
            remaining = regex_subst(remaining, "^(nats|tls):\\/\\/", "");
        }

        # Parse userinfo
        if (remaining =~ /@/) {
            string userinfo = (remaining =~ x/^([^@]+)@/)[0];
            remaining = regex_subst(remaining, "^[^@]+@", "");
            if (userinfo =~ /:/) {
                list<string> parts = userinfo.split(":");
                result.username = parts[0];
                result.password = parts[1];
            } else {
                result.username = userinfo;
            }
        }

        # Parse host:port
        if (remaining =~ /:/) {
            list<string> parts = remaining.split(":");
            result.host = parts[0];
            # Remove trailing path
            string port_str = regex_subst(parts[1], "/.*", "");
            result.port = int(port_str);
        } else {
            # Remove trailing path
            result.host = regex_subst(remaining, "/.*", "");
        }

        if (!result.host.val()) {
            result.host = "localhost";
        }

        return result;
    }
}

} # namespace Nats
