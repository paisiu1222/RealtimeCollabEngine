/** Message types matching network::MessageType enum */
export enum MessageType {
  CONNECT      = 'connect',
  DISCONNECT   = 'disconnect',
  HEARTBEAT    = 'heartbeat',
  JOIN_ROOM    = 'join_room',
  LEAVE_ROOM   = 'leave_room',
  ROOM_INFO    = 'room_info',
  OPERATION    = 'operation',
  OPERATION_ACK = 'operation_ack',
  DOCUMENT_SYNC = 'document_sync',
  USER_JOIN    = 'user_join',
  USER_LEAVE   = 'user_leave',
  CURSOR_UPDATE = 'cursor_update',
  ERROR        = 'error',
}

/** Operation types matching core::OperationType */
export enum OpType {
  INSERT  = 'insert',
  DELETE  = 'delete',
  REPLACE = 'replace',
  RETAIN  = 'retain',
}

/** Wire message format matching network::Message */
export interface WireMessage {
  type: string;
  messageId: string;
  timestamp: string;
  payload: Record<string, unknown>;
}

/** Edit operation matching core::Operation + payload shape */
export interface EditOp {
  opId: string;
  userId: string;
  version: number;
  type: OpType;
  position: number;
  content: string;
}

/** Cursor position for a user */
export interface CursorState {
  userId: string;
  position: number;
  selectionStart: number;
  selectionEnd: number;
}

/** User presence info */
export interface UserInfo {
  userId: string;
  username: string;
  cursor?: CursorState | null;
  online: boolean;
}

/** Room info from server */
export interface RoomInfo {
  roomId: string;
  documentId: string;
  userCount: number;
  version: number;
}

/** Version snapshot — Git-like commit point */
export interface Version {
  id: string;
  docId: string;
  version: number;
  content: string;
  message: string;
  timestamp: string;
  opCount: number;
  isAuto: boolean;
}

/** Single line in a unified diff view */
export interface DiffLine {
  type: 'unchanged' | 'added' | 'deleted';
  lineNumber: { old?: number; new?: number };
  text: string;
}

// --- message factory helpers ---

let seq = 0;
function genId(): string {
  return `${Date.now()}-${++seq}-${Math.random().toString(36).slice(2, 10)}`;
}
function nowISO(): string {
  return new Date().toISOString();
}
function wrap(type: MessageType, payload: Record<string, unknown>): WireMessage {
  return { type, messageId: genId(), timestamp: nowISO(), payload };
}

export const Msg = {
  connect: (userId: string, token: string) =>
    wrap(MessageType.CONNECT, { userId, token }),

  joinRoom: (roomId: string) =>
    wrap(MessageType.JOIN_ROOM, { roomId }),

  leaveRoom: (roomId: string) =>
    wrap(MessageType.LEAVE_ROOM, { roomId }),

  heartbeat: () =>
    wrap(MessageType.HEARTBEAT, { ping: true }),

  heartbeatResp: () =>
    wrap(MessageType.HEARTBEAT, { pong: true }),

  operation: (op: EditOp) =>
    wrap(MessageType.OPERATION, {
      opId: op.opId,
      userId: op.userId,
      version: op.version,
      operation: { type: op.type, position: op.position, content: op.content },
    }),

  cursorUpdate: (cursor: CursorState) =>
    wrap(MessageType.CURSOR_UPDATE, { ...cursor }),

  disconnect: (reason = '') =>
    wrap(MessageType.DISCONNECT, { reason }),

  error: (code: string, message: string) =>
    wrap(MessageType.ERROR, { code, message }),
};
