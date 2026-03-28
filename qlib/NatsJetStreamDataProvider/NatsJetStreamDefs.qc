# -*- mode: qore; indent-tabs-mode: nil -*-
#! @file NatsJetStreamDefs.qc NATS JetStream data provider constants and types

/*  NatsJetStreamDefs.qc Copyright 2026 Qore Technologies, s.r.o.

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
    #! Event ID for a received JetStream message
    public const EVENT_JS_MESSAGE = "jetstream-message";

    #! Event ID for JetStream subscription completion
    public const EVENT_JS_COMPLETE = "jetstream-complete";

    #! Event ID for a JetStream error
    public const EVENT_JS_ERROR = "jetstream-error";

    #! JetStream retention policy
    public enum NatsJSRetentionPolicy {
        #! Messages are retained based on limits (max_msgs, max_bytes, max_age)
        JS_RETENTION_LIMITS = 0,
        #! Messages are removed when all known consumers have acknowledged
        JS_RETENTION_INTEREST = 1,
        #! Messages are removed when any consumer acknowledges (work queue pattern)
        JS_RETENTION_WORKQUEUE = 2,
    }

    #! JetStream storage type
    public enum NatsJSStorageType {
        #! Store messages on disk
        JS_STORAGE_FILE = 0,
        #! Store messages in memory
        JS_STORAGE_MEMORY = 1,
    }

    #! JetStream discard policy
    public enum NatsJSDiscardPolicy {
        #! Discard old messages when limits are reached
        JS_DISCARD_OLD = 0,
        #! Reject new messages when limits are reached
        JS_DISCARD_NEW = 1,
    }

    #! JetStream acknowledgment policy
    public enum NatsJSAckPolicy {
        #! No acknowledgment required
        JS_ACK_NONE = 0,
        #! Acknowledge all prior messages
        JS_ACK_ALL = 1,
        #! Each message must be individually acknowledged
        JS_ACK_EXPLICIT = 2,
    }

    #! JetStream deliver policy
    public enum NatsJSDeliverPolicy {
        #! Deliver all messages
        JS_DELIVER_ALL = 0,
        #! Deliver starting with the last message
        JS_DELIVER_LAST = 1,
        #! Deliver only new messages
        JS_DELIVER_NEW = 2,
        #! Deliver starting at a specific sequence
        JS_DELIVER_BY_START_SEQUENCE = 3,
        #! Deliver starting at a specific time
        JS_DELIVER_BY_START_TIME = 4,
        #! Deliver the last message per subject
        JS_DELIVER_LAST_PER_SUBJECT = 5,
    }

    #! JetStream replay policy
    public enum NatsJSReplayPolicy {
        #! Replay messages as fast as possible
        JS_REPLAY_INSTANT = 0,
        #! Replay messages at the original rate
        JS_REPLAY_ORIGINAL = 1,
    }

    #! Stream configuration request data type (for create/update)
    public class NatsJSStreamConfigDataType inherits HashDataType {
        public {
            const Fields = {
                "name": {
                    "type": StringType,
                    "display_name": "Stream Name",
                    "short_desc": "Name of the stream",
                    "desc": "Unique identifier for the JetStream stream.\n\n"
                        "Stream names are used to reference the stream in all JetStream operations "
                        "such as publishing, subscribing, and management. **Names must be "
                        "alphanumeric and may include dashes or underscores but not periods or "
                        "spaces.**",
                    "required": True,
                    "example_value": "ORDERS",
                },
                "subjects": {
                    "type": SoftListOrNothingType,
                    "display_name": "Subjects",
                    "short_desc": "Subjects the stream listens to",
                    "desc": "List of NATS subjects that this stream will capture messages from.\n\n"
                        "Supports wildcards:\n"
                        "- `*` matches a single token (e.g. `orders.*`)\n"
                        "- `>` matches one or more tokens (e.g. `orders.>`)\n\n"
                        "Multiple subjects can be specified to aggregate messages from different "
                        "topic hierarchies into a single stream.",
                    "example_value": ("orders.>",),
                },
                "retention": {
                    "type": IntOrNothingType,
                    "display_name": "Retention Policy",
                    "short_desc": "Stream retention policy",
                    "desc": "Controls how messages are retained or removed from the stream.\n\n"
                        "- `0` (Limits): messages are kept until stream limits (`max_msgs`, "
                        "`max_bytes`, `max_age`) are reached\n"
                        "- `1` (Interest): messages are removed once all known consumers have "
                        "acknowledged them\n"
                        "- `2` (Work Queue): messages are removed as soon as any consumer "
                        "acknowledges them\n\n"
                        "**Default is `0` (Limits) if not specified.**",
                    "allowed_values": (
                        <AllowedValueInfo>{"value": 0, "display_name": "Limits",
                            "short_desc": "Retain based on limits (max_msgs, max_bytes, max_age)"},
                        <AllowedValueInfo>{"value": 1, "display_name": "Interest",
                            "short_desc": "Remove when all known consumers have acknowledged"},
                        <AllowedValueInfo>{"value": 2, "display_name": "Work Queue",
                            "short_desc": "Remove when any consumer acknowledges"},
                    ),
                },
                "max_msgs": {
                    "type": IntOrNothingType,
                    "display_name": "Max Messages",
                    "short_desc": "Maximum number of messages",
                    "desc": "Maximum number of messages the stream will retain.\n\n"
                        "When this limit is reached, older messages are discarded according to "
                        "the `discard` policy. Set to `-1` for unlimited messages.\n\n"
                        "**Only applies when the `retention` policy is set to Limits (`0`).**",
                    "example_value": -1,
                },
                "max_bytes": {
                    "type": IntOrNothingType,
                    "display_name": "Max Bytes",
                    "short_desc": "Maximum total bytes",
                    "desc": "Maximum total storage in bytes for all messages in the stream.\n\n"
                        "When this limit is reached, older messages are discarded according to "
                        "the `discard` policy. Set to `-1` for unlimited storage.\n\n"
                        "**Only applies when the `retention` policy is set to Limits (`0`).**",
                    "example_value": -1,
                },
                "max_age": {
                    "type": IntOrNothingType,
                    "display_name": "Max Age (ms)",
                    "short_desc": "Maximum message age in milliseconds",
                    "desc": "Maximum age of any message in the stream, specified in milliseconds.\n\n"
                        "Messages older than this value are automatically removed. Set to `0` "
                        "for no age limit. For example, `86400000` sets a 24-hour retention "
                        "window.\n\n"
                        "**Only applies when the `retention` policy is set to Limits (`0`).**",
                    "example_value": 86400000,
                },
                "max_msg_size": {
                    "type": IntOrNothingType,
                    "display_name": "Max Message Size",
                    "short_desc": "Maximum message size in bytes",
                    "desc": "Maximum size in bytes for any individual message published to the "
                        "stream.\n\n"
                        "Messages exceeding this size will be rejected by the server. Set to "
                        "`-1` to use the server's default maximum (typically 1 MB). For example, "
                        "`1048576` sets a 1 MB limit.",
                    "example_value": 1048576,
                },
                "storage": {
                    "type": IntOrNothingType,
                    "display_name": "Storage Type",
                    "short_desc": "Message storage type",
                    "desc": "Controls where stream messages are persisted.\n\n"
                        "- `0` (File): messages are stored on disk for durability across server "
                        "restarts\n"
                        "- `1` (Memory): messages are stored in memory for higher throughput but "
                        "are lost on server restart\n\n"
                        "**Default is `0` (File) if not specified.**",
                    "allowed_values": (
                        <AllowedValueInfo>{"value": 0, "display_name": "File",
                            "short_desc": "Store messages on disk"},
                        <AllowedValueInfo>{"value": 1, "display_name": "Memory",
                            "short_desc": "Store messages in memory"},
                    ),
                },
                "num_replicas": {
                    "type": IntOrNothingType,
                    "display_name": "Replicas",
                    "short_desc": "Number of replicas",
                    "desc": "Number of copies of the stream data to maintain across a NATS "
                        "cluster for high availability.\n\n"
                        "A value of `1` means no replication (single copy). Values of `3` or "
                        "`5` are common for production clusters. **Must be an odd number and "
                        "cannot exceed the number of servers in the cluster.**",
                    "example_value": 1,
                },
                "discard": {
                    "type": IntOrNothingType,
                    "display_name": "Discard Policy",
                    "short_desc": "Discard policy when limits are reached",
                    "desc": "Controls which messages are removed when stream limits "
                        "(`max_msgs`, `max_bytes`, or `max_age`) are reached.\n\n"
                        "- `0` (Old): the oldest messages are removed to make room\n"
                        "- `1` (New): new messages are rejected with an error\n\n"
                        "**Default is `0` (Old) if not specified.**",
                    "allowed_values": (
                        <AllowedValueInfo>{"value": 0, "display_name": "Old",
                            "short_desc": "Discard old messages when limits are reached"},
                        <AllowedValueInfo>{"value": 1, "display_name": "New",
                            "short_desc": "Reject new messages when limits are reached"},
                    ),
                },
                "description": {
                    "type": StringOrNothingType,
                    "display_name": "Description",
                    "short_desc": "Stream description",
                    "desc": "Optional human-readable description for the stream.\n\n"
                        "This is a free-form text field that can be used to document the "
                        "stream's purpose, the types of messages it contains, or any other "
                        "relevant operational context.",
                    "example_value": "Order processing stream",
                },
            };
        }
        constructor() : HashDataType("NatsJSStreamConfig") {
            addQoreFields(Fields);
        }
    }

    public const NatsJSStreamConfigDataType = new NatsJSStreamConfigDataType();

    #! Consumer configuration request data type
    public class NatsJSConsumerConfigDataType inherits HashDataType {
        public {
            const Fields = {
                "stream": {
                    "type": StringType,
                    "display_name": "Stream Name",
                    "short_desc": "Name of the stream",
                    "desc": "The name of the JetStream stream that this consumer will read "
                        "messages from.\n\n"
                        "The stream must already exist before the consumer can be created. Use "
                        "the stream `name` value from the stream configuration.",
                    "required": True,
                    "example_value": "ORDERS",
                },
                "durable_name": {
                    "type": StringOrNothingType,
                    "display_name": "Durable Name",
                    "short_desc": "Durable consumer name",
                    "desc": "A unique name that makes the consumer durable (persistent).\n\n"
                        "**Important**: without a durable name, the consumer is ephemeral and "
                        "will be automatically removed when it disconnects or becomes inactive. "
                        "A durable consumer retains its state (delivery position, pending "
                        "acknowledgments) across client disconnects and server restarts.",
                    "example_value": "order-processor",
                },
                "deliver_subject": {
                    "type": StringOrNothingType,
                    "display_name": "Deliver Subject",
                    "short_desc": "Subject for push delivery",
                    "desc": "The NATS subject where the server will push messages to the "
                        "consumer.\n\n"
                        "If set, the consumer operates in **push mode** where the server "
                        "actively delivers messages to this subject. If not set, the consumer "
                        "operates in **pull mode** where clients must explicitly request "
                        "messages.",
                },
                "deliver_group": {
                    "type": StringOrNothingType,
                    "display_name": "Deliver Group",
                    "short_desc": "Deliver group (queue group)",
                    "desc": "Queue group name for load-balanced message delivery across "
                        "multiple consumer instances.\n\n"
                        "When set, messages are distributed across all consumers sharing the "
                        "same `deliver_group` value, ensuring each message is processed by "
                        "only one consumer. **Only applicable in push mode** (when "
                        "`deliver_subject` is set).",
                },
                "ack_policy": {
                    "type": IntOrNothingType,
                    "display_name": "Ack Policy",
                    "short_desc": "Acknowledgment policy",
                    "desc": "Controls how the consumer acknowledges message delivery.\n\n"
                        "- `0` (None): no acknowledgment required; messages are considered "
                        "delivered immediately\n"
                        "- `1` (All): acknowledging a message implicitly acknowledges all "
                        "prior messages\n"
                        "- `2` (Explicit): each message must be individually acknowledged\n\n"
                        "**Default is `2` (Explicit) if not specified.**",
                    "allowed_values": (
                        <AllowedValueInfo>{"value": 0, "display_name": "None",
                            "short_desc": "No acknowledgment required"},
                        <AllowedValueInfo>{"value": 1, "display_name": "All",
                            "short_desc": "Acknowledge all prior messages"},
                        <AllowedValueInfo>{"value": 2, "display_name": "Explicit",
                            "short_desc": "Each message must be individually acknowledged"},
                    ),
                },
                "ack_wait": {
                    "type": IntOrNothingType,
                    "display_name": "Ack Wait (ms)",
                    "short_desc": "Acknowledgment wait timeout in milliseconds",
                    "desc": "Duration in milliseconds that the server will wait for an "
                        "acknowledgment before considering a message unacknowledged and "
                        "scheduling it for redelivery.\n\n"
                        "For example, `30000` sets a 30-second timeout. **If not specified, "
                        "the server default of 30 seconds is used.** Set this value based on "
                        "the expected processing time for your messages.",
                    "example_value": 30000,
                },
                "deliver_policy": {
                    "type": IntOrNothingType,
                    "display_name": "Deliver Policy",
                    "short_desc": "Message delivery policy",
                    "desc": "Determines the starting point for message delivery to the "
                        "consumer.\n\n"
                        "- `0` (All): deliver all available messages from the beginning of "
                        "the stream\n"
                        "- `1` (Last): start with the last message in the stream\n"
                        "- `2` (New): deliver only messages published after the consumer is "
                        "created\n"
                        "- `3` (By Start Sequence): start at a specific sequence number\n"
                        "- `4` (By Start Time): start at a specific timestamp\n"
                        "- `5` (Last Per Subject): deliver the last message for each subject\n"
                        "\n**Default is `0` (All) if not specified.**",
                    "allowed_values": (
                        <AllowedValueInfo>{"value": 0, "display_name": "All",
                            "short_desc": "Deliver all messages"},
                        <AllowedValueInfo>{"value": 1, "display_name": "Last",
                            "short_desc": "Deliver starting with the last message"},
                        <AllowedValueInfo>{"value": 2, "display_name": "New",
                            "short_desc": "Deliver only new messages"},
                        <AllowedValueInfo>{"value": 3, "display_name": "By Start Sequence",
                            "short_desc": "Deliver starting at a specific sequence"},
                        <AllowedValueInfo>{"value": 4, "display_name": "By Start Time",
                            "short_desc": "Deliver starting at a specific time"},
                        <AllowedValueInfo>{"value": 5, "display_name": "Last Per Subject",
                            "short_desc": "Deliver the last message per subject"},
                    ),
                },
                "replay_policy": {
                    "type": IntOrNothingType,
                    "display_name": "Replay Policy",
                    "short_desc": "Message replay policy",
                    "desc": "Controls the speed at which messages are replayed to the consumer "
                        "when catching up on historical messages.\n\n"
                        "- `0` (Instant): messages are delivered as fast as possible\n"
                        "- `1` (Original): messages are delivered at the rate they were "
                        "originally published, preserving the original timing\n\n"
                        "**Default is `0` (Instant) if not specified.**",
                    "allowed_values": (
                        <AllowedValueInfo>{"value": 0, "display_name": "Instant",
                            "short_desc": "Replay messages as fast as possible"},
                        <AllowedValueInfo>{"value": 1, "display_name": "Original",
                            "short_desc": "Replay messages at the original rate"},
                    ),
                },
                "filter_subject": {
                    "type": StringOrNothingType,
                    "display_name": "Filter Subject",
                    "short_desc": "Filter subject for the consumer",
                    "desc": "A subject filter that restricts the consumer to receiving only "
                        "messages matching this pattern.\n\n"
                        "This allows a consumer to process a subset of the stream's messages. "
                        "Supports NATS wildcard tokens (`*` and `>`). If not set, the consumer "
                        "receives all messages from the stream.",
                    "example_value": "orders.created",
                },
                "max_deliver": {
                    "type": IntOrNothingType,
                    "display_name": "Max Deliver",
                    "short_desc": "Maximum delivery attempts",
                    "desc": "Maximum number of times the server will attempt to deliver a "
                        "message to the consumer.\n\n"
                        "After this many failed delivery attempts, the message is dropped or "
                        "sent to a dead-letter queue if configured. Set to `-1` for unlimited "
                        "redelivery attempts. **A value of `5` is commonly used in "
                        "production.**",
                    "example_value": 5,
                },
                "max_ack_pending": {
                    "type": IntOrNothingType,
                    "display_name": "Max Ack Pending",
                    "short_desc": "Maximum pending acknowledgments",
                    "desc": "Maximum number of messages that can be delivered but not yet "
                        "acknowledged.\n\n"
                        "This acts as a flow control mechanism to prevent the consumer from "
                        "being overwhelmed. Once this limit is reached, the server pauses "
                        "delivery until some messages are acknowledged. **Default is `1000` "
                        "if not specified.**",
                    "example_value": 1000,
                },
                "description": {
                    "type": StringOrNothingType,
                    "display_name": "Description",
                    "short_desc": "Consumer description",
                    "desc": "Optional human-readable description for the consumer.\n\n"
                        "This is a free-form text field that can be used to document the "
                        "consumer's purpose, the processing logic it implements, or any "
                        "other relevant operational context.",
                    "example_value": "Process incoming orders",
                },
            };
        }
        constructor() : HashDataType("NatsJSConsumerConfig") {
            addQoreFields(Fields);
        }
    }

    public const NatsJSConsumerConfigDataType = new NatsJSConsumerConfigDataType();

    #! Stream info response data type
    public class NatsJSStreamInfoDataType inherits HashDataType {
        public {
            const Fields = {
                "config": {
                    "type": AutoHashType,
                    "display_name": "Configuration",
                    "short_desc": "Stream configuration",
                    "desc": "The full stream configuration hash containing all settings such "
                        "as `name`, `subjects`, `retention`, `storage`, and limits.\n\n"
                        "This is the same configuration that was provided when the stream was "
                        "created or last updated.",
                },
                "messages": {
                    "type": IntType,
                    "display_name": "Messages",
                    "short_desc": "Number of messages in stream",
                    "desc": "Total number of messages currently stored in the stream.\n\n"
                        "This count reflects the live state after retention policies and "
                        "consumer acknowledgments have been applied.",
                    "example_value": 1542,
                },
                "bytes": {
                    "type": IntType,
                    "display_name": "Bytes",
                    "short_desc": "Total bytes in stream",
                    "desc": "Total storage in bytes consumed by all messages currently in "
                        "the stream.\n\n"
                        "This includes message headers, payloads, and internal metadata "
                        "overhead.",
                    "example_value": 524288,
                },
                "first_seq": {
                    "type": IntType,
                    "display_name": "First Sequence",
                    "short_desc": "First sequence number",
                    "desc": "The sequence number of the oldest message still retained in "
                        "the stream.\n\n"
                        "This value increases as older messages are removed by retention "
                        "policies or explicit purge operations.",
                    "example_value": 1,
                },
                "last_seq": {
                    "type": IntType,
                    "display_name": "Last Sequence",
                    "short_desc": "Last sequence number",
                    "desc": "The sequence number of the most recently published message in "
                        "the stream.\n\n"
                        "This is a monotonically increasing value that represents the "
                        "stream's high-water mark.",
                    "example_value": 1542,
                },
                "consumer_count": {
                    "type": IntType,
                    "display_name": "Consumer Count",
                    "short_desc": "Number of consumers",
                    "desc": "The number of consumers currently bound to this stream.\n\n"
                        "Includes both durable and ephemeral consumers that are actively "
                        "registered.",
                    "example_value": 3,
                },
            };
        }
        constructor() : HashDataType("NatsJSStreamInfoResponse") {
            addQoreFields(Fields);
        }
    }

    public const NatsJSStreamInfoDataType = new NatsJSStreamInfoDataType();

    #! Consumer info response data type
    public class NatsJSConsumerInfoDataType inherits HashDataType {
        public {
            const Fields = {
                "config": {
                    "type": AutoHashType,
                    "display_name": "Configuration",
                    "short_desc": "Consumer configuration",
                    "desc": "The full consumer configuration hash containing all settings such "
                        "as `durable_name`, `ack_policy`, `deliver_policy`, and filters.\n\n"
                        "This is the same configuration that was provided when the consumer was "
                        "created or last updated.",
                },
                "delivered": {
                    "type": IntType,
                    "display_name": "Delivered",
                    "short_desc": "Number of delivered messages",
                    "desc": "Total number of messages that have been delivered to the "
                        "consumer since it was created.\n\n"
                        "This includes messages that were subsequently redelivered, so it may "
                        "be higher than the number of unique messages processed.",
                    "example_value": 5000,
                },
                "ack_pending": {
                    "type": IntType,
                    "display_name": "Ack Pending",
                    "short_desc": "Number of pending acknowledgments",
                    "desc": "Number of messages that have been delivered but not yet "
                        "acknowledged by the consumer.\n\n"
                        "A consistently high value may indicate slow processing or stuck "
                        "consumers. This count is limited by the `max_ack_pending` "
                        "configuration.",
                    "example_value": 12,
                },
                "redelivered": {
                    "type": IntType,
                    "display_name": "Redelivered",
                    "short_desc": "Number of redelivered messages",
                    "desc": "Total number of messages that have been redelivered to the "
                        "consumer due to acknowledgment timeouts.\n\n"
                        "A high value may indicate that the `ack_wait` timeout is too short "
                        "or that the consumer is failing to process messages.",
                    "example_value": 3,
                },
                "waiting": {
                    "type": IntType,
                    "display_name": "Waiting",
                    "short_desc": "Number of waiting pull requests",
                    "desc": "Number of pull requests that are currently waiting for messages "
                        "to become available.\n\n"
                        "**Only applicable to pull-based consumers.** A value of `0` for a "
                        "push consumer is normal.",
                    "example_value": 0,
                },
                "num_pending": {
                    "type": IntType,
                    "display_name": "Pending",
                    "short_desc": "Number of pending messages",
                    "desc": "Number of messages in the stream that have not yet been "
                        "delivered to this consumer.\n\n"
                        "This represents the consumer's backlog. A growing value indicates "
                        "that the consumer is falling behind the rate of message production.",
                    "example_value": 150,
                },
            };
        }
        constructor() : HashDataType("NatsJSConsumerInfoResponse") {
            addQoreFields(Fields);
        }
    }

    public const NatsJSConsumerInfoDataType = new NatsJSConsumerInfoDataType();

    #! Publish acknowledgment response data type
    public class NatsJSPubAckDataType inherits HashDataType {
        public {
            const Fields = {
                "stream": {
                    "type": StringType,
                    "display_name": "Stream",
                    "short_desc": "Stream name",
                    "desc": "The name of the JetStream stream that the message was published "
                        "to.\n\n"
                        "Confirms the target stream, which is useful when publishing to "
                        "subjects that are captured by multiple streams.",
                    "example_value": "ORDERS",
                },
                "sequence": {
                    "type": IntType,
                    "display_name": "Sequence",
                    "short_desc": "Message sequence number",
                    "desc": "The monotonically increasing sequence number assigned to the "
                        "published message within the stream.\n\n"
                        "This can be used to track message ordering and to reference specific "
                        "messages for replay or debugging.",
                    "example_value": 42,
                },
                "duplicate": {
                    "type": BoolType,
                    "display_name": "Duplicate",
                    "short_desc": "Whether the message is a duplicate",
                    "desc": "Indicates whether the server detected this message as a "
                        "duplicate based on the `Nats-Msg-Id` header.\n\n"
                        "JetStream provides idempotent publishing by tracking message IDs "
                        "within a configurable deduplication window. If `True`, the message "
                        "was already stored and no new copy was created. The `sequence` value "
                        "will reference the original message.",
                    "example_value": False,
                },
            };
        }
        constructor() : HashDataType("NatsJSPubAckResponse") {
            addQoreFields(Fields);
        }
    }

    public const NatsJSPubAckDataType = new NatsJSPubAckDataType();

    #! JetStream message event data type
    public class NatsJSMessageDataType inherits HashDataType {
        public {
            const Fields = {
                "subject": {
                    "type": StringType,
                    "display_name": "Subject",
                    "short_desc": "The subject the message was published to",
                    "desc": "The NATS subject that the message was originally published to.\n\n"
                        "For streams listening on wildcard subjects, this will be the fully "
                        "resolved subject (e.g. `orders.created` rather than `orders.>`).",
                    "example_value": "orders.created",
                },
                "data": {
                    "type": DataOrNothingType,
                    "display_name": "Data",
                    "short_desc": "The message payload",
                    "desc": "The binary message payload as raw bytes.\n\n"
                        "Applications typically serialize data as JSON, Protocol Buffers, or "
                        "other formats before publishing. The consumer is responsible for "
                        "deserializing the payload.",
                },
                "reply": {
                    "type": StringOrNothingType,
                    "display_name": "Reply Subject",
                    "short_desc": "The reply subject",
                    "desc": "The NATS reply subject used internally by JetStream for message "
                        "acknowledgment.\n\n"
                        "**This is managed automatically by the JetStream client** and should "
                        "not be used directly for request-reply patterns.",
                },
                "headers": {
                    "type": AutoHashOrNothingType,
                    "display_name": "Headers",
                    "short_desc": "Message headers",
                    "desc": "Optional key-value metadata headers attached to the message.\n\n"
                        "Headers can carry metadata such as content type, correlation IDs, or "
                        "tracing information without affecting the message payload.",
                },
            };
        }
        constructor() : HashDataType("NatsJSMessageEvent") {
            addQoreFields(Fields);
        }
    }

    public const NatsJSMessageDataType = new NatsJSMessageDataType();

    #! JetStream error event data type
    public class NatsJSErrorDataType inherits HashDataType {
        public {
            const Fields = {
                "err": {
                    "type": StringType,
                    "display_name": "Error Code",
                    "short_desc": "The error code",
                    "desc": "The NATS JetStream error code identifying the type of error "
                        "that occurred.\n\n"
                        "This can be used for programmatic error handling and retry logic.",
                    "example_value": "10037",
                },
                "desc": {
                    "type": StringType,
                    "display_name": "Description",
                    "short_desc": "The error description",
                    "desc": "A human-readable description of the error providing additional "
                        "context about what went wrong.\n\n"
                        "This message is intended for logging and diagnostics; use the `err` "
                        "code for programmatic error handling.",
                    "example_value": "no message found",
                },
            };
        }
        constructor() : HashDataType("NatsJSErrorEvent") {
            addQoreFields(Fields);
        }
    }

    public const NatsJSErrorDataType = new NatsJSErrorDataType();

    #! JetStream complete event data type
    public const NatsJSCompleteDataType = new HashDataType("NatsJSCompleteEvent");

    #! KV entry response data type
    public class NatsJSKVEntryDataType inherits HashDataType {
        public {
            const Fields = {
                "bucket": {
                    "type": StringType,
                    "display_name": "Bucket",
                    "short_desc": "KV bucket name",
                    "desc": "The name of the NATS JetStream Key-Value bucket that contains "
                        "this entry.\n\n"
                        "Buckets are logical namespaces for organizing related key-value pairs, "
                        "backed by a JetStream stream.",
                    "example_value": "config",
                },
                "key": {
                    "type": StringType,
                    "display_name": "Key",
                    "short_desc": "Key name",
                    "desc": "The key identifier within the bucket.\n\n"
                        "Keys can use dot-separated hierarchies (e.g. `app.database.host`) "
                        "to organize values logically. Keys are unique within a bucket.",
                    "example_value": "app.database.host",
                },
                "value": {
                    "type": DataOrNothingType,
                    "display_name": "Value",
                    "short_desc": "Key value (binary)",
                    "desc": "The binary value stored at this key.\n\n"
                        "Returns `NOTHING` for delete and purge operations (tombstone entries). "
                        "Applications typically store serialized data such as JSON or plain "
                        "text.",
                },
                "revision": {
                    "type": IntType,
                    "display_name": "Revision",
                    "short_desc": "Revision number",
                    "desc": "The monotonically increasing revision number for this key entry.\n\n"
                        "Every put, delete, or purge operation increments the revision. This "
                        "can be used for optimistic concurrency control by checking the "
                        "expected revision before updating a value.",
                    "example_value": 5,
                },
                "created": {
                    "type": DateType,
                    "display_name": "Created",
                    "short_desc": "Creation timestamp",
                    "desc": "The timestamp when this specific revision of the entry was "
                        "created.\n\n"
                        "This reflects the time of the most recent operation (put, delete, "
                        "or purge) on this key, not the original creation time.",
                },
                "operation": {
                    "type": IntOrNothingType,
                    "display_name": "Operation",
                    "short_desc": "Operation type",
                    "desc": "The type of operation that created this KV entry revision.\n\n"
                        "- `1` (Put): a value was stored at the key\n"
                        "- `2` (Delete): the key was soft-deleted, creating a tombstone that "
                        "preserves history\n"
                        "- `3` (Purge): the key and all its history were permanently removed",
                    "allowed_values": (
                        <AllowedValueInfo>{"value": 1, "display_name": "Put",
                            "short_desc": "A value was stored at the key"},
                        <AllowedValueInfo>{"value": 2, "display_name": "Delete",
                            "short_desc": "The key was soft-deleted (tombstone)"},
                        <AllowedValueInfo>{"value": 3, "display_name": "Purge",
                            "short_desc": "The key and all history were permanently removed"},
                    ),
                    "example_value": 1,
                },
            };
        }
        constructor() : HashDataType("NatsJSKVEntryResponse") {
            addQoreFields(Fields);
        }
    }

    public const NatsJSKVEntryDataType = new NatsJSKVEntryDataType();

    #! JetStream account info response data type
    public class NatsJSAccountInfoDataType inherits HashDataType {
        public {
            const Fields = {
                "memory": {
                    "type": IntType,
                    "display_name": "Memory",
                    "short_desc": "Memory used in bytes",
                    "desc": "Total memory used by JetStream in bytes.\n\n"
                        "This reflects the combined memory footprint of all in-memory streams "
                        "and their messages. Monitor this value against account limits to "
                        "avoid running out of memory.",
                    "example_value": 1048576,
                },
                "storage": {
                    "type": IntType,
                    "display_name": "Storage",
                    "short_desc": "Storage used in bytes",
                    "desc": "Total storage used by JetStream in bytes.\n\n"
                        "This reflects the combined disk footprint of all file-backed streams "
                        "and their messages. Monitor this value against account limits to "
                        "plan capacity.",
                    "example_value": 5242880,
                },
                "streams": {
                    "type": IntType,
                    "display_name": "Streams",
                    "short_desc": "Number of active streams",
                    "desc": "Total number of active JetStream streams in the account.\n\n"
                        "Includes both file-backed and memory-backed streams. Check against "
                        "account limits to determine remaining capacity.",
                    "example_value": 5,
                },
                "consumers": {
                    "type": IntType,
                    "display_name": "Consumers",
                    "short_desc": "Total number of consumers across all streams",
                    "desc": "Total number of consumers across all streams in the account.\n\n"
                        "Includes both durable and ephemeral consumers that are currently "
                        "registered.",
                    "example_value": 12,
                },
                "domain": {
                    "type": StringOrNothingType,
                    "display_name": "Domain",
                    "short_desc": "JetStream domain name for multi-domain setups",
                    "desc": "The JetStream domain name for multi-domain setups.\n\n"
                        "In a multi-domain JetStream deployment, this identifies which domain "
                        "the account belongs to. Returns `NOTHING` if no domain is configured.",
                },
                "api_total": {
                    "type": IntType,
                    "display_name": "API Total",
                    "short_desc": "Total number of JetStream API requests processed",
                    "desc": "Total number of JetStream API requests that have been processed "
                        "by the server for this account.\n\n"
                        "This is a cumulative counter since the server started and can be used "
                        "for monitoring API usage patterns.",
                    "example_value": 10000,
                },
                "api_errors": {
                    "type": IntType,
                    "display_name": "API Errors",
                    "short_desc": "Number of JetStream API errors",
                    "desc": "Total number of JetStream API requests that resulted in errors "
                        "for this account.\n\n"
                        "A rising error count may indicate misconfigured clients, permission "
                        "issues, or resource exhaustion.",
                    "example_value": 5,
                },
            };
        }
        constructor() : HashDataType("NatsJSAccountInfoResponse") {
            addQoreFields(Fields);
        }
    }

    public const NatsJSAccountInfoDataType = new NatsJSAccountInfoDataType();

    #! Consumer pause response data type
    public class NatsJSConsumerPauseResponseDataType inherits HashDataType {
        public {
            const Fields = {
                "paused": {
                    "type": BoolType,
                    "display_name": "Paused",
                    "short_desc": "Whether the consumer is currently paused",
                    "desc": "Indicates whether the consumer is currently in a paused state.\n\n"
                        "A paused consumer stops receiving messages until the `pause_until` "
                        "time is reached or the consumer is explicitly resumed.",
                    "example_value": True,
                },
                "pause_until": {
                    "type": DateOrNothingType,
                    "display_name": "Pause Until",
                    "short_desc": "When the consumer will automatically resume",
                    "desc": "The timestamp when the consumer will automatically resume "
                        "message delivery.\n\n"
                        "After this time, the consumer will start receiving messages again "
                        "without requiring manual intervention. Returns `NOTHING` if the "
                        "consumer is not paused.",
                },
                "pause_remaining_ms": {
                    "type": IntOrNothingType,
                    "display_name": "Pause Remaining (ms)",
                    "short_desc": "Remaining pause time in milliseconds",
                    "desc": "The remaining time in milliseconds until the consumer "
                        "automatically resumes.\n\n"
                        "Returns `NOTHING` if the consumer is not paused or the pause has "
                        "already expired.",
                    "example_value": 30000,
                },
            };
        }
        constructor() : HashDataType("NatsJSConsumerPauseResponse") {
            addQoreFields(Fields);
        }
    }

    public const NatsJSConsumerPauseResponseDataType = new NatsJSConsumerPauseResponseDataType();

    #! KV bucket status response data type
    public class NatsJSKVBucketStatusDataType inherits HashDataType {
        public {
            const Fields = {
                "bucket": {
                    "type": StringType,
                    "display_name": "Bucket",
                    "short_desc": "Bucket name",
                    "desc": "The name of the KV bucket.\n\n"
                        "This is the unique identifier used to reference the bucket in all "
                        "KV operations.",
                    "example_value": "config",
                },
                "values": {
                    "type": IntType,
                    "display_name": "Values",
                    "short_desc": "Number of unique keys in the bucket",
                    "desc": "The number of unique keys currently stored in the bucket.\n\n"
                        "This count excludes deleted keys that only have tombstone entries.",
                    "example_value": 42,
                },
                "history": {
                    "type": IntType,
                    "display_name": "History",
                    "short_desc": "Number of historical revisions kept per key",
                    "desc": "The maximum number of historical revisions that are kept per "
                        "key in this bucket.\n\n"
                        "When the history limit is reached, older revisions are automatically "
                        "removed.",
                    "example_value": 5,
                },
                "ttl_ms": {
                    "type": IntType,
                    "display_name": "TTL (ms)",
                    "short_desc": "Time-to-live for values in milliseconds",
                    "desc": "The time-to-live for values in the bucket, in milliseconds.\n\n"
                        "A value of `0` means no TTL is configured and values persist "
                        "indefinitely.",
                    "example_value": 0,
                },
                "replicas": {
                    "type": IntType,
                    "display_name": "Replicas",
                    "short_desc": "Number of replicas",
                    "desc": "The number of replicas configured for this bucket.\n\n"
                        "Higher replica counts provide better durability and availability "
                        "in a clustered NATS deployment.",
                    "example_value": 1,
                },
                "bytes": {
                    "type": IntType,
                    "display_name": "Bytes",
                    "short_desc": "Total bytes used by the bucket",
                    "desc": "Total storage in bytes consumed by all keys and their "
                        "revisions in this bucket.\n\n"
                        "This includes values, metadata, and internal overhead.",
                    "example_value": 1048576,
                },
            };
        }
        constructor() : HashDataType("NatsJSKVBucketStatusResponse") {
            addQoreFields(Fields);
        }
    }

    public const NatsJSKVBucketStatusDataType = new NatsJSKVBucketStatusDataType();

    #! KV bucket configuration request data type (for createKeyValue)
    public class NatsJSKVConfigRequestDataType inherits HashDataType {
        public {
            const Fields = {
                "bucket": {
                    "type": StringType,
                    "display_name": "Bucket Name",
                    "short_desc": "Bucket name to create",
                    "desc": "The name for the new KV bucket.\n\n"
                        "Bucket names must be alphanumeric and may include dashes or "
                        "underscores. The name must be unique within the JetStream account.",
                    "required": True,
                    "example_value": "config",
                },
                "description": {
                    "type": StringOrNothingType,
                    "display_name": "Description",
                    "short_desc": "Bucket description",
                    "desc": "Optional human-readable description for the bucket.\n\n"
                        "This is a free-form text field that can be used to document the "
                        "bucket's purpose and the types of data it stores.",
                },
                "max_value_size": {
                    "type": IntOrNothingType,
                    "display_name": "Max Value Size",
                    "short_desc": "Maximum value size in bytes",
                    "desc": "Maximum size in bytes for any individual value stored in the "
                        "bucket.\n\n"
                        "Values exceeding this size will be rejected. If not set, the server "
                        "default is used.",
                },
                "history": {
                    "type": IntOrNothingType,
                    "display_name": "History",
                    "short_desc": "Number of historical values to keep per key",
                    "desc": "The number of historical revisions to retain for each key.\n\n"
                        "Allows retrieving previous values via the history API. A value of "
                        "`1` means only the latest revision is kept. **Default is `1` if not "
                        "specified.**",
                    "example_value": 5,
                },
                "ttl": {
                    "type": IntOrNothingType,
                    "display_name": "TTL (ms)",
                    "short_desc": "Time-to-live for values in milliseconds",
                    "desc": "Time-to-live for values in the bucket, in milliseconds.\n\n"
                        "Values older than this duration are automatically removed. Set to "
                        "`0` or omit for no TTL (values persist indefinitely).",
                },
                "max_bytes": {
                    "type": IntOrNothingType,
                    "display_name": "Max Bytes",
                    "short_desc": "Maximum total bytes in the bucket",
                    "desc": "Maximum total storage in bytes for all keys and revisions in "
                        "the bucket.\n\n"
                        "When this limit is reached, old revisions are removed to make room. "
                        "If not set, the server default is used.",
                },
                "storage": {
                    "type": IntOrNothingType,
                    "display_name": "Storage Type",
                    "short_desc": "Storage type",
                    "desc": "Controls where bucket data is persisted.\n\n"
                        "- `0` (File): data is stored on disk for durability across server "
                        "restarts\n"
                        "- `1` (Memory): data is stored in memory for higher throughput but "
                        "is lost on server restart\n\n"
                        "**Default is `0` (File) if not specified.**",
                    "allowed_values": (
                        <AllowedValueInfo>{"value": 0, "display_name": "File",
                            "short_desc": "Store data on disk"},
                        <AllowedValueInfo>{"value": 1, "display_name": "Memory",
                            "short_desc": "Store data in memory"},
                    ),
                },
                "num_replicas": {
                    "type": IntOrNothingType,
                    "display_name": "Replicas",
                    "short_desc": "Number of replicas",
                    "desc": "Number of copies of the bucket data to maintain across a NATS "
                        "cluster for high availability.\n\n"
                        "A value of `1` means no replication (single copy). **Must be an odd "
                        "number and cannot exceed the number of servers in the cluster.**",
                    "example_value": 1,
                },
            };
        }
        constructor() : HashDataType("NatsJSKVConfigRequest") {
            addQoreFields(Fields);
        }
    }

    public const NatsJSKVConfigRequestDataType = new NatsJSKVConfigRequestDataType();

    #! Stream list response data type
    public class NatsJSStreamListResponseDataType inherits HashDataType {
        public {
            const Fields = {
                "streams": {
                    "type": SoftListType,
                    "display_name": "Streams",
                    "short_desc": "List of stream information hashes",
                    "desc": "A list of stream information hashes, one per stream.\n\n"
                        "Each entry contains the stream's configuration and current state "
                        "including message count and byte size.",
                },
            };
        }
        constructor() : HashDataType("NatsJSStreamListResponse") {
            addQoreFields(Fields);
        }
    }

    public const NatsJSStreamListResponseDataType = new NatsJSStreamListResponseDataType();

    #! Stream names response data type
    public class NatsJSStreamNamesResponseDataType inherits HashDataType {
        public {
            const Fields = {
                "names": {
                    "type": SoftListType,
                    "display_name": "Names",
                    "short_desc": "List of stream names",
                    "desc": "A list of stream name strings.\n\n"
                        "Returns only the names without full stream configuration or state. "
                        "Use this for lightweight enumeration when detailed information is "
                        "not needed.",
                },
            };
        }
        constructor() : HashDataType("NatsJSStreamNamesResponse") {
            addQoreFields(Fields);
        }
    }

    public const NatsJSStreamNamesResponseDataType = new NatsJSStreamNamesResponseDataType();

    #! Consumer list response data type
    public class NatsJSConsumerListResponseDataType inherits HashDataType {
        public {
            const Fields = {
                "consumers": {
                    "type": SoftListType,
                    "display_name": "Consumers",
                    "short_desc": "List of consumer information hashes",
                    "desc": "A list of consumer information hashes for the specified "
                        "stream.\n\n"
                        "Each entry contains the consumer's configuration and current "
                        "delivery state including pending messages and acknowledgment "
                        "progress.",
                },
            };
        }
        constructor() : HashDataType("NatsJSConsumerListResponse") {
            addQoreFields(Fields);
        }
    }

    public const NatsJSConsumerListResponseDataType = new NatsJSConsumerListResponseDataType();

    #! Consumer names response data type
    public class NatsJSConsumerNamesResponseDataType inherits HashDataType {
        public {
            const Fields = {
                "names": {
                    "type": SoftListType,
                    "display_name": "Names",
                    "short_desc": "List of consumer names",
                    "desc": "A list of consumer name strings for the specified stream.\n\n"
                        "Returns only the names without full consumer configuration or "
                        "state. Use this for lightweight enumeration when detailed "
                        "information is not needed.",
                },
            };
        }
        constructor() : HashDataType("NatsJSConsumerNamesResponse") {
            addQoreFields(Fields);
        }
    }

    public const NatsJSConsumerNamesResponseDataType = new NatsJSConsumerNamesResponseDataType();

    #! Stream message request data type (for getMsg)
    public class NatsJSStreamMsgRequestDataType inherits HashDataType {
        public {
            const Fields = {
                "stream": {
                    "type": StringType,
                    "display_name": "Stream Name",
                    "short_desc": "Stream name",
                    "desc": "The name of the JetStream stream to retrieve the message "
                        "from.\n\n"
                        "The stream must already exist; an error is raised if the stream "
                        "is not found.",
                    "required": True,
                    "example_value": "ORDERS",
                },
                "seq": {
                    "type": IntType,
                    "display_name": "Sequence Number",
                    "short_desc": "Message sequence number",
                    "desc": "The sequence number of the message to retrieve.\n\n"
                        "Sequence numbers are assigned monotonically by the stream. Use "
                        "stream info to determine the valid range (`first_seq` to "
                        "`last_seq`).",
                    "required": True,
                    "example_value": 42,
                },
            };
        }
        constructor() : HashDataType("NatsJSStreamMsgRequest") {
            addQoreFields(Fields);
        }
    }

    public const NatsJSStreamMsgRequestDataType = new NatsJSStreamMsgRequestDataType();

    #! Stream message by subject request data type (for getLastMsg)
    public class NatsJSStreamMsgBySubjectRequestDataType inherits HashDataType {
        public {
            const Fields = {
                "stream": {
                    "type": StringType,
                    "display_name": "Stream Name",
                    "short_desc": "Stream name",
                    "desc": "The name of the JetStream stream to retrieve the message "
                        "from.\n\n"
                        "The stream must already exist; an error is raised if the stream "
                        "is not found.",
                    "required": True,
                    "example_value": "ORDERS",
                },
                "subject": {
                    "type": StringType,
                    "display_name": "Subject",
                    "short_desc": "Subject to get last message for",
                    "desc": "The subject to retrieve the last message for.\n\n"
                        "Returns the most recently published message on this subject "
                        "within the stream. The subject must match one of the stream's "
                        "configured subject filters.",
                    "required": True,
                    "example_value": "orders.created",
                },
            };
        }
        constructor() : HashDataType("NatsJSStreamMsgBySubjectRequest") {
            addQoreFields(Fields);
        }
    }

    public const NatsJSStreamMsgBySubjectRequestDataType = new NatsJSStreamMsgBySubjectRequestDataType();
}
