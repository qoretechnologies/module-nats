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
                    "desc": "The JetStream stream name",
                    "required": True,
                    "example_value": "ORDERS",
                },
                "subjects": {
                    "type": SoftListOrNothingType,
                    "display_name": "Subjects",
                    "short_desc": "Subjects the stream listens to",
                    "desc": "List of subjects the stream listens to",
                    "example_value": ("orders.>",),
                },
                "retention": {
                    "type": IntOrNothingType,
                    "display_name": "Retention Policy",
                    "short_desc": "Stream retention policy",
                    "desc": "The retention policy for the stream",
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
                    "desc": "Maximum number of messages in the stream",
                },
                "max_bytes": {
                    "type": IntOrNothingType,
                    "display_name": "Max Bytes",
                    "short_desc": "Maximum total bytes",
                    "desc": "Maximum total bytes for the stream",
                },
                "max_age": {
                    "type": IntOrNothingType,
                    "display_name": "Max Age (ms)",
                    "short_desc": "Maximum message age in milliseconds",
                    "desc": "Maximum message age in milliseconds (`0` = unlimited)",
                },
                "max_msg_size": {
                    "type": IntOrNothingType,
                    "display_name": "Max Message Size",
                    "short_desc": "Maximum message size in bytes",
                    "desc": "Maximum individual message size in bytes",
                },
                "storage": {
                    "type": IntOrNothingType,
                    "display_name": "Storage Type",
                    "short_desc": "Message storage type",
                    "desc": "The storage type for stream messages",
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
                    "desc": "Number of replicas for the stream in a clustered environment",
                },
                "discard": {
                    "type": IntOrNothingType,
                    "display_name": "Discard Policy",
                    "short_desc": "Discard policy when limits are reached",
                    "desc": "The discard policy when stream limits are reached",
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
                    "desc": "Optional description for the stream",
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
                    "desc": "The JetStream stream name to create the consumer on",
                    "required": True,
                    "example_value": "ORDERS",
                },
                "durable_name": {
                    "type": StringOrNothingType,
                    "display_name": "Durable Name",
                    "short_desc": "Durable consumer name",
                    "desc": "Durable consumer name; if set, the consumer survives restarts",
                    "example_value": "order-processor",
                },
                "deliver_subject": {
                    "type": StringOrNothingType,
                    "display_name": "Deliver Subject",
                    "short_desc": "Subject for push delivery",
                    "desc": "Subject for push-based message delivery",
                },
                "deliver_group": {
                    "type": StringOrNothingType,
                    "display_name": "Deliver Group",
                    "short_desc": "Deliver group (queue group)",
                    "desc": "Deliver group for load-balanced delivery across consumers",
                },
                "ack_policy": {
                    "type": IntOrNothingType,
                    "display_name": "Ack Policy",
                    "short_desc": "Acknowledgment policy",
                    "desc": "The acknowledgment policy for the consumer",
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
                    "desc": "How long the server waits for an acknowledgment before redelivering",
                },
                "deliver_policy": {
                    "type": IntOrNothingType,
                    "display_name": "Deliver Policy",
                    "short_desc": "Message delivery policy",
                    "desc": "Controls which messages are delivered to the consumer",
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
                    "desc": "Controls how messages are replayed to the consumer",
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
                    "desc": "Filter subject to receive a subset of stream messages",
                    "example_value": "orders.created",
                },
                "max_deliver": {
                    "type": IntOrNothingType,
                    "display_name": "Max Deliver",
                    "short_desc": "Maximum delivery attempts",
                    "desc": "Maximum number of delivery attempts for a message",
                },
                "max_ack_pending": {
                    "type": IntOrNothingType,
                    "display_name": "Max Ack Pending",
                    "short_desc": "Maximum pending acknowledgments",
                    "desc": "Maximum number of outstanding acknowledgments",
                },
                "description": {
                    "type": StringOrNothingType,
                    "display_name": "Description",
                    "short_desc": "Consumer description",
                    "desc": "Optional description for the consumer",
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
                    "desc": "The stream configuration",
                },
                "messages": {
                    "type": IntType,
                    "display_name": "Messages",
                    "short_desc": "Number of messages in stream",
                    "desc": "Number of messages currently in the stream",
                },
                "bytes": {
                    "type": IntType,
                    "display_name": "Bytes",
                    "short_desc": "Total bytes in stream",
                    "desc": "Total bytes of all messages in the stream",
                },
                "first_seq": {
                    "type": IntType,
                    "display_name": "First Sequence",
                    "short_desc": "First sequence number",
                    "desc": "The first sequence number in the stream",
                },
                "last_seq": {
                    "type": IntType,
                    "display_name": "Last Sequence",
                    "short_desc": "Last sequence number",
                    "desc": "The last sequence number in the stream",
                },
                "consumer_count": {
                    "type": IntType,
                    "display_name": "Consumer Count",
                    "short_desc": "Number of consumers",
                    "desc": "Number of consumers on the stream",
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
                    "desc": "The consumer configuration",
                },
                "delivered": {
                    "type": IntType,
                    "display_name": "Delivered",
                    "short_desc": "Number of delivered messages",
                    "desc": "Number of messages delivered to the consumer",
                },
                "ack_pending": {
                    "type": IntType,
                    "display_name": "Ack Pending",
                    "short_desc": "Number of pending acknowledgments",
                    "desc": "Number of pending acknowledgments",
                },
                "redelivered": {
                    "type": IntType,
                    "display_name": "Redelivered",
                    "short_desc": "Number of redelivered messages",
                    "desc": "Number of redelivered messages",
                },
                "waiting": {
                    "type": IntType,
                    "display_name": "Waiting",
                    "short_desc": "Number of waiting pull requests",
                    "desc": "Number of waiting pull requests",
                },
                "num_pending": {
                    "type": IntType,
                    "display_name": "Pending",
                    "short_desc": "Number of pending messages",
                    "desc": "Number of pending messages to be delivered",
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
                    "desc": "The stream the message was published to",
                },
                "sequence": {
                    "type": IntType,
                    "display_name": "Sequence",
                    "short_desc": "Message sequence number",
                    "desc": "The sequence number assigned to the published message",
                },
                "duplicate": {
                    "type": BoolType,
                    "display_name": "Duplicate",
                    "short_desc": "Whether the message is a duplicate",
                    "desc": "Whether the message was detected as a duplicate",
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
                    "desc": "The subject the message was published to",
                },
                "data": {
                    "type": DataOrNothingType,
                    "display_name": "Data",
                    "short_desc": "The message payload",
                    "desc": "The binary message payload",
                },
                "reply": {
                    "type": StringOrNothingType,
                    "display_name": "Reply Subject",
                    "short_desc": "The reply subject",
                    "desc": "The reply subject for acknowledgment",
                },
                "headers": {
                    "type": AutoHashOrNothingType,
                    "display_name": "Headers",
                    "short_desc": "Message headers",
                    "desc": "Optional message headers as key-value pairs",
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
                    "desc": "The error code",
                },
                "desc": {
                    "type": StringType,
                    "display_name": "Description",
                    "short_desc": "The error description",
                    "desc": "The error description",
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
                    "desc": "The Key-Value bucket name",
                },
                "key": {
                    "type": StringType,
                    "display_name": "Key",
                    "short_desc": "Key name",
                    "desc": "The key name",
                },
                "value": {
                    "type": DataOrNothingType,
                    "display_name": "Value",
                    "short_desc": "Key value (binary)",
                    "desc": "The value stored at this key",
                },
                "revision": {
                    "type": IntType,
                    "display_name": "Revision",
                    "short_desc": "Revision number",
                    "desc": "The revision number for this entry",
                },
                "created": {
                    "type": DateType,
                    "display_name": "Created",
                    "short_desc": "Creation timestamp",
                    "desc": "The timestamp when this entry was created",
                },
                "operation": {
                    "type": IntOrNothingType,
                    "display_name": "Operation",
                    "short_desc": "Operation type",
                    "desc": "The operation type for this entry",
                    "allowed_values": (
                        <AllowedValueInfo>{"value": 1, "display_name": "Put"},
                        <AllowedValueInfo>{"value": 2, "display_name": "Delete"},
                        <AllowedValueInfo>{"value": 3, "display_name": "Purge"},
                    ),
                },
            };
        }
        constructor() : HashDataType("NatsJSKVEntryResponse") {
            addQoreFields(Fields);
        }
    }

    public const NatsJSKVEntryDataType = new NatsJSKVEntryDataType();
}
