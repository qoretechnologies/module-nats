# -*- mode: qore; indent-tabs-mode: nil -*-
#! @file NatsClient.qc High-level NATS client

/*  NatsClient.qc Copyright 2026 Qore Technologies, s.r.o.

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

#! High-level NATS client with convenience methods
/** Wraps @ref Qore::Nats::NatsConnection with additional convenience methods
    including string publish/request, channel-based subscriptions, and
    callback-based subscriptions.

    @par Example:
    @code{.py}
NatsClient client("nats://localhost:4222");
client.publishString("events.log", "hello world");

NatsChannelSubscription nsub = client.channelSubscribe("events.>", 100);
foreach hash<NatsMsgInfo> msg in (nsub) {
    printf("Got: %s\n", msg.subject);
}
    @endcode

    @since NatsUtil 1.0
*/
public class NatsClient {
    private {
        #! The underlying NATS connection
        NatsConnection conn;
    }

    #! Creates a NatsClient from a URL
    /** @param url NATS server URL (e.g., "nats://localhost:4222")
    */
    constructor(string url) {
        conn = new NatsConnection(url);
    }

    #! Creates a NatsClient from connection options
    /** @param options connection options (see @ref NatsConnectionOptions)
    */
    constructor(hash<NatsConnectionOptions> options) {
        conn = new NatsConnection(options);
    }

    #! Destructor — closes the connection
    destructor() {
        conn.close();
    }

    #! Publishes a binary message
    /** @param subject the subject to publish to
        @param data optional binary payload
    */
    publish(string subject, *binary data) {
        conn.publish(subject, data);
    }

    #! Publishes a string message
    /** @param subject the subject to publish to
        @param data the string payload
    */
    publishString(string subject, string data) {
        conn.publishString(subject, data);
    }

    #! Publishes a message with headers
    /** @param subject the subject to publish to
        @param data optional binary payload
        @param headers optional message headers
    */
    publishMsg(string subject, *binary data, *hash<string, string> headers) {
        conn.publishMsg(subject, data, headers);
    }

    #! Sends a request and returns the reply as a string
    /** @param subject the subject to send the request to
        @param data the string payload
        @param timeout_ms timeout in milliseconds
        @return the reply string
    */
    string requestString(string subject, string data, timeout timeout_ms = 5s) {
        hash<NatsMsgInfo> reply = conn.request(subject, binary(data), timeout_ms);
        return reply.data ? reply.data.toString() : "";
    }

    #! Sends a request and returns the reply
    /** @param subject the subject to send the request to
        @param data optional binary payload
        @param timeout_ms timeout in milliseconds
        @return a NatsMsgInfo hash
    */
    hash<NatsMsgInfo> request(string subject, *binary data, timeout timeout_ms = 5s) {
        return conn.request(subject, data, timeout_ms);
    }

    #! Creates a channel-based subscription
    /** @param subject the subject to subscribe to
        @param buffer_size the channel buffer size
        @return a NatsChannelSubscription for iteration
    */
    NatsChannelSubscription channelSubscribe(string subject, int buffer_size = 100) {
        NatsSubscription nsub = conn.subscribe(subject);
        return new NatsChannelSubscription(nsub, buffer_size);
    }

    #! Creates a channel-based queue subscription
    /** @param subject the subject to subscribe to
        @param queue the queue group name
        @param buffer_size the channel buffer size
        @return a NatsChannelSubscription for iteration
    */
    NatsChannelSubscription channelQueueSubscribe(string subject, string queue,
            int buffer_size = 100) {
        NatsSubscription nsub = conn.queueSubscribe(subject, queue);
        return new NatsChannelSubscription(nsub, buffer_size);
    }

    #! Creates a callback-based subscription
    /** Messages are delivered to the callback in a background thread using
        a Channel internally.

        @param subject the subject to subscribe to
        @param callback the callback closure to invoke for each message
        @param buffer_size the channel buffer size
        @return the NatsChannelSubscription (for lifecycle management)
    */
    NatsChannelSubscription subscribe(string subject,
            code<nothing(hash<NatsMsgInfo>)> callback, int buffer_size = 100) {
        NatsChannelSubscription csub = channelSubscribe(subject, buffer_size);
        background callbackLoop(csub, callback);
        return csub;
    }

    #! Returns connection statistics
    /** @return a @ref NatsConnectionStats hash with message and byte counts
    */
    hash<NatsConnectionStats> getStats() {
        return conn.getStats();
    }

    #! Returns comprehensive connection information
    /** @return a @ref NatsConnectionInfo hash with all connection details
    */
    hash<NatsConnectionInfo> getConnectionInfo() {
        return conn.getConnectionInfo();
    }

    #! Returns True if the connection is active
    bool isConnected() {
        return conn.isConnected();
    }

    #! Returns the underlying NatsConnection
    NatsConnection getConnection() {
        return conn;
    }

    #! Drains the connection
    drain() {
        conn.drain();
    }

    #! Closes the connection
    close() {
        conn.close();
    }

    #! Flushes pending data
    flush(timeout timeout_ms = 5s) {
        conn.flush(timeout_ms);
    }

    #! Background loop for callback-based subscriptions
    private callbackLoop(NatsChannelSubscription csub, code<nothing(hash<NatsMsgInfo>)> callback) {
        try {
            foreach hash<NatsMsgInfo> msg in (csub.iterator()) {
                try {
                    callback(msg);
                } catch (hash<ExceptionInfo> ex) {
                    @debug("NatsClient::callbackLoop() callback error: %s: %s",
                        ex.err, ex.desc);
                }
            }
        } catch (hash<ExceptionInfo> ex) {
            @debug("NatsClient::callbackLoop() iteration error: %s: %s",
                ex.err, ex.desc);
        }
    }
}

} # namespace Nats
