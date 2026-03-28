# -*- mode: qore; indent-tabs-mode: nil -*-
#! @file NatsChannelKVWatch.qc Channel-based KV watcher wrapper

/*  NatsChannelKVWatch.qc Copyright 2026 Qore Technologies, s.r.o.

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

#! Channel-based KV watcher for async iteration over KV changes
/** Wraps a @ref Qore::Nats::NatsKeyValueWatcher with a @ref Channel for idiomatic
    async delivery of key-value change notifications following the Go/Rust/Python
    iterator pattern.

    A background thread calls \c next() in a loop and sends entries into a
    buffered channel. Consumer code iterates the channel using \c foreach, which
    blocks until an entry arrives or the channel is closed.

    @par Example: Iterate KV changes
    @code{.py}
NatsKeyValueStore kv = js.keyValue("config");
NatsKeyValueWatcher watcher = kv.watchAll(<NatsKVWatchOptions>{"updates_only": True});
NatsChannelKVWatch watch = new NatsChannelKVWatch(watcher);
foreach hash<NatsKVEntry> entry in (watch.iterator()) {
    printf("Key %s changed: %s\n", entry.key, entry.value.toString());
}
    @endcode

    @par Example: Receive with timeout
    @code{.py}
*hash<NatsKVEntry> entry = watch.recv(5s);
    @endcode

    @par Example: Multiplexing with channel_select()
    @code{.py}
auto result = channel_select(
    {"type": "recv", "channel": watch1.channel()},
    {"type": "recv", "channel": watch2.channel()},
);
    @endcode

    @since NatsUtil 1.0
*/
public class NatsChannelKVWatch {
    private {
        #! The underlying KV watcher
        NatsKeyValueWatcher watcher;

        #! The entry channel
        Channel chan;

        #! Stop flag
        bool stopping = False;

        #! Mutex for state synchronization
        Mutex m();
    }

    #! Creates a channel-based KV watcher
    /** @param watcher the underlying NatsKeyValueWatcher
        @param buffer_size the channel buffer size (default: 100)
    */
    constructor(NatsKeyValueWatcher watcher, int buffer_size = 100) {
        self.watcher = watcher;
        self.chan = new Channel(buffer_size);
        background self.bgLoop();
    }

    #! Destructor -- ensures cleanup
    destructor() {
        close();
    }

    #! Returns an iterator for use with \c foreach
    /** @return a ChannelIterator that yields hash<NatsKVEntry> values
    */
    AbstractIterator iterator() {
        return chan.iterator();
    }

    #! Receives a single KV entry with timeout
    /** @param timeout_ms the timeout duration
        @return a hash<NatsKVEntry>, or NOTHING on timeout or closed channel
    */
    *hash<NatsKVEntry> recv(timeout timeout_ms = 5s) {
        try {
            return chan.recv(timeout_ms);
        } catch (hash<ExceptionInfo> ex) {
            if (ex.err == "CHANNEL-CLOSED" || ex.err == "CHANNEL-TIMEOUT") {
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

    #! Closes the watcher and channel
    close() {
        {
            AutoLock al(m);
            if (stopping) {
                return;
            }
            stopping = True;
        }
        try {
            watcher.stop();
        } catch (hash<ExceptionInfo> ex) {
            # Ignore errors if watcher is already stopped
        }
        try {
            chan.close();
        } catch () {
            # Channel may already be closed
        }
    }

    #! Returns True if the watcher is still active
    bool isActive() {
        AutoLock al(m);
        return !stopping;
    }

    #! Background thread that reads KV entries and sends to the channel
    private bgLoop() {
        @debug("NatsChannelKVWatch::bgLoop() started");
        on_exit {
            @debug("NatsChannelKVWatch::bgLoop() exiting");
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
                *hash<NatsKVEntry> entry = watcher.next(500ms);
                if (entry) {
                    try {
                        chan.send(entry);
                    } catch (hash<ExceptionInfo> ex) {
                        if (ex.err == "CHANNEL-CLOSED") {
                            break;
                        }
                        rethrow;
                    }
                }
            } catch (hash<ExceptionInfo> ex) {
                if (ex.err == "NATS-KV-WATCH-ERROR") {
                    # Watcher has been stopped or is invalid
                    break;
                }
                # Log unexpected errors and exit the loop gracefully
                @debug("NatsChannelKVWatch::bgLoop() unexpected error: %s: %s",
                    ex.err, ex.desc);
                break;
            }
        }
    }
}

} # namespace Nats
