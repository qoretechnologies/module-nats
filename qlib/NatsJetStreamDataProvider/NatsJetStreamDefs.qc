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
    public const NatsJSStreamConfigDataType = new HashDataType("NatsJSStreamConfig", {
        "name": {
            "type": StringType,
            "desc": "Stream name",
            "required": True,
        },
        "subjects": {
            "type": SoftListOrNothingType,
            "desc": "Subjects the stream listens to",
        },
        "retention": {
            "type": IntOrNothingType,
            "desc": "Retention policy (see NatsJSRetentionPolicy enum)",
        },
        "max_msgs": {
            "type": IntOrNothingType,
            "desc": "Maximum number of messages",
        },
        "max_bytes": {
            "type": IntOrNothingType,
            "desc": "Maximum total bytes",
        },
        "max_age": {
            "type": IntOrNothingType,
            "desc": "Maximum message age in milliseconds (0 = unlimited)",
        },
        "max_msg_size": {
            "type": IntOrNothingType,
            "desc": "Maximum message size",
        },
        "storage": {
            "type": IntOrNothingType,
            "desc": "Storage type (see NatsJSStorageType enum)",
        },
        "num_replicas": {
            "type": IntOrNothingType,
            "desc": "Number of replicas",
        },
        "discard": {
            "type": IntOrNothingType,
            "desc": "Discard policy (see NatsJSDiscardPolicy enum)",
        },
        "description": {
            "type": StringOrNothingType,
            "desc": "Stream description",
        },
    });

    #! Consumer configuration request data type
    public const NatsJSConsumerConfigDataType = new HashDataType("NatsJSConsumerConfig", {
        "stream": {
            "type": StringType,
            "desc": "Stream name",
            "required": True,
        },
        "durable_name": {
            "type": StringOrNothingType,
            "desc": "Durable consumer name",
        },
        "deliver_subject": {
            "type": StringOrNothingType,
            "desc": "Subject for push delivery",
        },
        "deliver_group": {
            "type": StringOrNothingType,
            "desc": "Deliver group (queue group)",
        },
        "ack_policy": {
            "type": IntOrNothingType,
            "desc": "Ack policy (see NatsJSAckPolicy enum)",
        },
        "ack_wait": {
            "type": IntOrNothingType,
            "desc": "Ack wait in milliseconds",
        },
        "deliver_policy": {
            "type": IntOrNothingType,
            "desc": "Deliver policy (see NatsJSDeliverPolicy enum)",
        },
        "filter_subject": {
            "type": StringOrNothingType,
            "desc": "Filter subject",
        },
        "max_deliver": {
            "type": IntOrNothingType,
            "desc": "Maximum delivery attempts",
        },
        "max_ack_pending": {
            "type": IntOrNothingType,
            "desc": "Maximum pending acks",
        },
        "description": {
            "type": StringOrNothingType,
            "desc": "Consumer description",
        },
    });

    #! Stream info response data type
    public const NatsJSStreamInfoDataType = new HashDataType("NatsJSStreamInfoResponse", {
        "config": {
            "type": AutoHashType,
            "desc": "Stream configuration",
        },
        "messages": {
            "type": IntType,
            "desc": "Number of messages in stream",
        },
        "bytes": {
            "type": IntType,
            "desc": "Total bytes in stream",
        },
        "first_seq": {
            "type": IntType,
            "desc": "First sequence number",
        },
        "last_seq": {
            "type": IntType,
            "desc": "Last sequence number",
        },
        "consumer_count": {
            "type": IntType,
            "desc": "Number of consumers",
        },
    });

    #! Consumer info response data type
    public const NatsJSConsumerInfoDataType = new HashDataType("NatsJSConsumerInfoResponse", {
        "config": {
            "type": AutoHashType,
            "desc": "Consumer configuration",
        },
        "delivered": {
            "type": IntType,
            "desc": "Number of delivered messages",
        },
        "ack_pending": {
            "type": IntType,
            "desc": "Number of pending acks",
        },
        "redelivered": {
            "type": IntType,
            "desc": "Number of redelivered messages",
        },
        "waiting": {
            "type": IntType,
            "desc": "Number of waiting pull requests",
        },
        "num_pending": {
            "type": IntType,
            "desc": "Number of pending messages",
        },
    });

    #! Publish acknowledgment response data type
    public const NatsJSPubAckDataType = new HashDataType("NatsJSPubAckResponse", {
        "stream": {
            "type": StringType,
            "desc": "Stream name",
        },
        "sequence": {
            "type": IntType,
            "desc": "Sequence number",
        },
        "duplicate": {
            "type": BoolType,
            "desc": "Whether the message is a duplicate",
        },
    });

    #! JetStream message event data type
    public const NatsJSMessageDataType = new HashDataType("NatsJSMessageEvent", {
        "subject": {
            "type": StringType,
            "desc": "The subject the message was published to",
        },
        "data": {
            "type": DataOrNothingType,
            "desc": "The message payload",
        },
        "reply": {
            "type": StringOrNothingType,
            "desc": "The reply subject",
        },
        "headers": {
            "type": AutoHashOrNothingType,
            "desc": "Message headers",
        },
    });

    #! JetStream error event data type
    public const NatsJSErrorDataType = new HashDataType("NatsJSErrorEvent", {
        "err": {
            "type": StringType,
            "desc": "The error code",
        },
        "desc": {
            "type": StringType,
            "desc": "The error description",
        },
    });

    #! JetStream complete event data type
    public const NatsJSCompleteDataType = new HashDataType("NatsJSCompleteEvent");

    #! KV entry response data type
    public const NatsJSKVEntryDataType = new HashDataType("NatsJSKVEntryResponse", {
        "bucket": {
            "type": StringType,
            "desc": "Bucket name",
        },
        "key": {
            "type": StringType,
            "desc": "Key name",
        },
        "value": {
            "type": DataOrNothingType,
            "desc": "Value (binary)",
        },
        "revision": {
            "type": IntType,
            "desc": "Revision number",
        },
        "created": {
            "type": DateType,
            "desc": "Creation timestamp",
        },
        "operation": {
            "type": IntOrNothingType,
            "desc": "Operation type (NATS_KV_OP_PUT = 1, NATS_KV_OP_DELETE = 2, NATS_KV_OP_PURGE = 3)",
        },
    });
}
