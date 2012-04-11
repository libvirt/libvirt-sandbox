
const GVIR_SANDBOX_PROTOCOL_PACKET_MAX = 262144;
const GVIR_SANDBOX_PROTOCOL_LEN_MAX = 4;
const GVIR_SANDBOX_PROTOCOL_HEADER_MAX = 16;
const GVIR_SANDBOX_PROTOCOL_PAYLOAD_MAX = 262128;

const GVIR_SANDBOX_PROTOCOL_HANDSHAKE_WAIT = 033;
const GVIR_SANDBOX_PROTOCOL_HANDSHAKE_SYNC = 034;

enum GVirSandboxProtocolProc {
     GVIR_SANDBOX_PROTOCOL_PROC_STDIN = 1,
     GVIR_SANDBOX_PROTOCOL_PROC_STDOUT = 2,
     GVIR_SANDBOX_PROTOCOL_PROC_STDERR = 3,
     GVIR_SANDBOX_PROTOCOL_PROC_EXIT = 4,
     GVIR_SANDBOX_PROTOCOL_PROC_QUIT = 5
};

enum GVirSandboxProtocolType {
    /* Async message */
    GVIR_SANDBOX_PROTOCOL_TYPE_MESSAGE = 0,
    /* Async data packet */
    GVIR_SANDBOX_PROTOCOL_TYPE_DATA = 1
};

enum GVirSandboxProtocolStatus {
    /* Status is always GVIR_SANDBOX_PROTO_OK for calls.
     * For replies, indicates no error.
     */
    GVIR_SANDBOX_PROTOCOL_STATUS_OK = 0
};

struct GVirSandboxProtocolHeader {
    GVirSandboxProtocolProc proc;
    GVirSandboxProtocolType type;
    GVirSandboxProtocolStatus status;
    unsigned serial;
};

struct GVirSandboxProtocolMessageExit {
     int status;
};
