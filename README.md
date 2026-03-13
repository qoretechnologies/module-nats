# Qore NATS Module

Qore module for [NATS](https://nats.io/) messaging, providing core publish/subscribe, request/reply, JetStream persistent messaging, and Key-Value store support.

## Features

- **Core NATS**: publish/subscribe, request/reply, queue groups, wildcard subscriptions
- **JetStream**: stream and consumer management, publish with acknowledgment, push and pull subscribe
- **Key-Value Store**: put, get, create, update (CAS), delete, purge, list keys, history
- **TLS**: encrypted connections with mutual TLS support
- **Authentication**: token, username/password, NKey, and credentials file
- **Channel-based subscriptions**: Go-style async iteration with `foreach` and `channel_select()`
- **Data providers**: `NatsDataProvider` and `NatsJetStreamDataProvider` for Qore data provider framework integration
- **Connection providers**: `nats://` and `tls://` connection schemes

## Requirements

- Qore 2.0+
- [nats.c](https://github.com/nats-io/nats.c) library (libnats)

## Building

```bash
mkdir build && cd build
cmake ..
make
sudo make install
```

To install to a specific prefix:

```bash
cmake -DCMAKE_INSTALL_PREFIX=/usr/local ..
```

## Quick Start

### Publish / Subscribe

```qore
#!/usr/bin/env qr

%requires NatsUtil

NatsClient client("nats://localhost:4222");

# Subscribe using channel-based async subscription
NatsChannelSubscription sub = client.channelSubscribe("events.>");

# Publish a message
client.publishString("events.log", "Hello NATS!");

# Receive the message
*hash<NatsMsgInfo> msg = sub.recv(5s);
if (msg) {
    printf("Received on %s: %s\n", msg.subject, msg.data.toString());
}

sub.close();
client.close();
```

### Request / Reply

```qore
#!/usr/bin/env qr

%requires NatsUtil

NatsClient client("nats://localhost:4222");

# Set up a responder in the background
NatsChannelSubscription sub = client.channelSubscribe("service.echo");
background sub () {
    foreach hash<NatsMsgInfo> msg in (sub) {
        if (msg.reply) {
            client.publishString(msg.reply, "Echo: " + msg.data.toString());
        }
    }
}();

# Send a request and wait for the reply
string reply = client.requestString("service.echo", "Hello!", 5s);
printf("Reply: %s\n", reply);

sub.close();
client.close();
```

### Queue Groups

```qore
#!/usr/bin/env qr

%requires NatsUtil

NatsClient client("nats://localhost:4222");

# Create queue subscribers -- messages are load-balanced across the group
NatsChannelSubscription sub1 = client.channelQueueSubscribe("work.tasks", "workers");
NatsChannelSubscription sub2 = client.channelQueueSubscribe("work.tasks", "workers");

# Publish tasks -- each goes to only one worker
for (int i = 0; i < 10; ++i) {
    client.publishString("work.tasks", sprintf("task-%d", i));
}

sub1.close();
sub2.close();
client.close();
```

### Wildcard Subscriptions

```qore
#!/usr/bin/env qr

%requires NatsUtil

NatsClient client("nats://localhost:4222");

# '*' matches a single token
NatsChannelSubscription sub1 = client.channelSubscribe("orders.*");

# '>' matches one or more tokens (must be last)
NatsChannelSubscription sub2 = client.channelSubscribe("events.>");

client.publishString("orders.new", "order-1");       # matched by sub1
client.publishString("events.log.error", "oops");    # matched by sub2

sub1.close();
sub2.close();
client.close();
```

### Callback-Based Subscriptions

```qore
#!/usr/bin/env qr

%requires NatsUtil

NatsClient client("nats://localhost:4222");

# Subscribe with a callback — messages are delivered in a background thread
NatsChannelSubscription sub = client.subscribe("events.>", sub (hash<NatsMsgInfo> msg) {
    printf("Got %s: %s\n", msg.subject, msg.data.toString());
});

# Do other work while messages are processed in the background
sleep(10s);

sub.close();
client.close();
```

### Error Handling

NATS operations raise exceptions on failure. Exception codes follow the pattern `NATS-<CATEGORY>-ERROR`:

| Exception | Description |
|---|---|
| `NATS-CONNECTION-ERROR` | Connection or configuration failures |
| `NATS-PUBLISH-ERROR` | Publish failures |
| `NATS-SUBSCRIBE-ERROR` | Subscription failures |
| `NATS-REQUEST-ERROR` | Request/reply failures |
| `NATS-TIMEOUT-ERROR` | Operation timeouts |
| `NATS-JETSTREAM-ERROR` | JetStream failures (includes JetStream error code) |
| `NATS-KV-ERROR` | Key-Value store failures |
| `NATS-AUTH-ERROR` | Authentication failures |
| `NATS-TLS-ERROR` | TLS/SSL failures |

```qore
#!/usr/bin/env qr

%requires NatsUtil

try {
    NatsClient client("nats://invalid-host:4222");
} catch (hash<ExceptionInfo> ex) {
    if (ex.err == "NATS-CONNECTION-ERROR") {
        printf("Failed to connect: %s\n", ex.desc);
    }
}

NatsClient client("nats://localhost:4222");
try {
    string reply = client.requestString("service.echo", "hello", 500ms);
} catch (hash<ExceptionInfo> ex) {
    if (ex.err == "NATS-TIMEOUT-ERROR") {
        printf("Request timed out: %s\n", ex.desc);
    }
}
```

### NatsConnection vs NatsClient

The module provides two levels of API:

- **NatsConnection** (binary module): Low-level connection with binary publish/subscribe and direct JetStream/KV access. Use when you need direct control or binary data.
- **NatsClient** (NatsUtil module): High-level client with string operations, channel-based async subscriptions, and callback subscriptions. Use for most applications.

`NatsClient.getConnection()` returns the underlying `NatsConnection` for accessing JetStream and KV APIs.

### JetStream Publish / Subscribe

```qore
#!/usr/bin/env qr

%requires nats
%requires NatsUtil

NatsClient client("nats://localhost:4222");
JetStreamContext js = client.getConnection().jetStream();

# Create a stream
js.addStream(<NatsStreamConfig>{
    "name": "ORDERS",
    "subjects": ("orders.>",),
});

# Publish with acknowledgment
hash<NatsPubAck> ack = js.publish("orders.new", binary("order-123"));
printf("Published to stream %s, seq %d\n", ack.stream, ack.sequence);

# Subscribe and receive
NatsSubscription sub = js.subscribe("orders.>");
*hash<NatsMsgInfo> msg = sub.nextMsg(5s);
if (msg) {
    printf("Received: %s\n", msg.data.toString());
    sub.ack();
}

sub.unsubscribe();
js.deleteStream("ORDERS");
client.close();
```

### Stream and Consumer Management

```qore
#!/usr/bin/env qr

%requires nats

NatsConnection conn("nats://localhost:4222");
JetStreamContext js = conn.jetStream();

# Create a stream with configuration
hash<NatsStreamInfo> info = js.addStream(<NatsStreamConfig>{
    "name": "EVENTS",
    "subjects": ("events.>",),
    "retention": NATS_RETENTION_LIMITS,
    "storage": NATS_STORAGE_FILE,
    "max_msgs": 1000000,
});

# Create a durable consumer
hash<NatsConsumerInfo> ci = js.addConsumer("EVENTS", <NatsConsumerConfig>{
    "durable_name": "event-processor",
    "ack_policy": NATS_ACK_EXPLICIT,
    "deliver_policy": NATS_DELIVER_ALL,
});

# Get stream/consumer info
info = js.getStreamInfo("EVENTS");
ci = js.getConsumerInfo("EVENTS", "event-processor");

# Cleanup
js.deleteConsumer("EVENTS", "event-processor");
js.deleteStream("EVENTS");
conn.close();
```

### Key-Value Store

```qore
#!/usr/bin/env qr

%requires nats
%requires NatsUtil

NatsClient client("nats://localhost:4222");
JetStreamContext js = client.getConnection().jetStream();

# Create a KV bucket
NatsKeyValueStore kv = js.createKeyValue(<NatsKVConfig>{
    "bucket": "config",
    "history": 5,
});

# Put values
int rev1 = kv.putString("app.name", "MyApp");
int rev2 = kv.putString("app.version", "1.0");

# Get a value
*hash<NatsKVEntry> entry = kv.get("app.name");
if (entry) {
    printf("Key: %s, Value: %s, Rev: %d\n",
        entry.key, entry.value.toString(), entry.revision);
}

# CAS update
int rev3 = kv.update("app.name", binary("NewApp"), rev1);

# List keys and get history
list<string> keys = kv.keys();
list<hash<NatsKVEntry>> hist = kv.history("app.name");

# Cleanup
js.deleteKeyValue("config");
client.close();
```

### TLS Connections

```qore
#!/usr/bin/env qr

%requires nats

# TLS with server verification
NatsConnection conn(<NatsConnectionOptions>{
    "url": "tls://nats.example.com:4222",
    "tls": <NatsTlsOptions>{
        "ca_cert": "/path/to/ca.crt",
    },
});

# Mutual TLS with client certificate
NatsConnection conn2(<NatsConnectionOptions>{
    "url": "tls://nats.example.com:4222",
    "tls": <NatsTlsOptions>{
        "ca_cert": "/path/to/ca.crt",
        "client_cert": "/path/to/client.crt",
        "client_key": "/path/to/client.key",
    },
});
```

### Data Provider CLI Usage

```bash
# Core NATS: publish a message
qdp 'nats{url=nats://localhost:4222}/publish' dor subject=events.log,data=hello

# Core NATS: request/reply
qdp 'nats{url=nats://localhost:4222}/request' dor subject=service.echo,data=hello

# JetStream: create a stream
qdp 'natsjetstream{url=nats://localhost:4222}/streams/create' dor name=ORDERS,subjects=orders.>

# JetStream: publish with ack
qdp 'natsjetstream{url=nats://localhost:4222}/publish' dor subject=orders.new,data=order-123

# JetStream: KV put
qdp 'natsjetstream{url=nats://localhost:4222}/kv/put' dor bucket=config,key=app.name,value=MyApp

# JetStream: KV get
qdp 'natsjetstream{url=nats://localhost:4222}/kv/get' dor bucket=config,key=app.name
```

## License

MIT -- see [COPYING.MIT](COPYING.MIT) for details.

Copyright (C) 2026 Qore Technologies, s.r.o.
