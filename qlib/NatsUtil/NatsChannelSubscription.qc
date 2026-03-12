# -*- mode: qore; indent-tabs-mode: nil -*-
#! @file NatsChannelSubscription.qc Channel-based async subscription wrapper

/*  NatsChannelSubscription.qc Copyright 2026 Qore Technologies, s.r.o.

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

#! Channel-based async subscription wrapper
/** Wraps a @ref Qore::Nats::NatsSubscription with a @ref Channel for idiomatic
    async message delivery following the Go/Rust/Python iterator pattern.

    A background thread calls \c nextMsg() in a loop and sends messages into a
    buffered channel. Consumer code iterates the channel using \c foreach, which
    blocks until a message arrives or the channel is closed.

    @par Example: Iterate messages
    @code{.py}
NatsClient client("nats://localhost:4222");
NatsChannelSubscription sub = client.channelSubscribe("orders.>", 100);
foreach hash<NatsMsgInfo> msg in (sub) {
    process(msg);
}
    @endcode

    @par Example: Receive with timeout
    @code{.py}
*hash<NatsMsgInfo> msg = sub.recv(5s);
    @endcode

    @par Example: Multiplexing
    @code{.py}
auto result = channel_select(
    {"type": "recv", "channel": sub1.channel()},
    {"type": "recv", "channel": sub2.channel()},
);
    @endcode

    @since NatsUtil 1.0
*/
public class NatsChannelSubscription {
    private {
        #! The underlying NATS subscription
        NatsSubscription sub;

        #! The message channel
        Channel chan;

        #! Stop flag
        bool stopping = False;

        #! Background thread started flag
        bool started = False;

        #! Mutex for state synchronization
        Mutex m();
    }

    #! Creates a channel subscription
    /** @param sub the underlying NatsSubscription
        @param buffer_size the channel buffer size (default: 100)
    */
    constructor(NatsSubscription sub, int buffer_size = 100) {
        self.sub = sub;
        self.chan = new Channel(buffer_size);
        self.started = True;
        background self.bgLoop();
    }

    #! Destructor — ensures cleanup
    destructor() {
        close();
    }

    #! Returns an iterator for use with \c foreach
    /** @return a ChannelIterator that yields hash<NatsMsgInfo> values
    */
    AbstractIterator iterator() {
        return chan.iterator();
    }

    #! Receives a single message with timeout
    /** @param timeout the timeout duration
        @return a hash<NatsMsgInfo>, or NOTHING on timeout or closed channel
    */
    *hash<NatsMsgInfo> recv(timeout timeout = 5s) {
        try {
            return chan.get(timeout);
        } catch (hash<ExceptionInfo> ex) {
            if (ex.err == "CHANNEL-CLOSED-ERROR" || ex.err == "CHANNEL-TIMEOUT-ERROR") {
                return NOTHING;
            }
            rethrow;
        }
    }

    #! Returns the raw channel for use with \c channel_select()
    /** @return the underlying Channel object
    */
    Channel channel() {
        return chan;
    }

    #! Closes the subscription and channel
    close() {
        {
            AutoLock al(m);
            if (stopping) {
                return;
            }
            stopping = True;
        }
        sub.unsubscribe();
        chan.close();
    }

    #! Drains the subscription (processes pending messages then closes)
    drain() {
        {
            AutoLock al(m);
            if (stopping) {
                return;
            }
            stopping = True;
        }
        sub.drain();
        # Channel will be closed by the background thread when it exits
    }

    #! Returns True if the subscription is still active
    bool isActive() {
        AutoLock al(m);
        return !stopping;
    }

    #! Background thread that reads messages and sends to the channel
    private bgLoop() {
        on_exit {
            try {
                chan.close();
            } catch () {
                # channel may already be closed
            }
        }

        while (True) {
            {
                AutoLock al(m);
                if (stopping) {
                    break;
                }
            }

            try {
                *hash<NatsMsgInfo> msg = sub.nextMsg(500ms);
                if (msg) {
                    try {
                        chan.send(msg);
                    } catch (hash<ExceptionInfo> ex) {
                        if (ex.err == "CHANNEL-CLOSED-ERROR") {
                            break;
                        }
                        rethrow;
                    }
                }
            } catch (hash<ExceptionInfo> ex) {
                if (ex.err == "NATS-SUBSCRIBE-ERROR") {
                    # Subscription has been unsubscribed or is invalid
                    break;
                }
                rethrow;
            }
        }
    }
}

} # namespace Nats
