import { Msg, MessageType, type WireMessage, type EditOp, type CursorState, type UserInfo, type RoomInfo } from './protocol';

export type ConnectionState = 'disconnected' | 'connecting' | 'connected' | 'authenticated';

export interface WsCallbacks {
  onStateChange: (state: ConnectionState) => void;
  onRemoteOp: (op: EditOp) => void;
  onAck: (opId: string, version: number) => void;
  onUserJoin: (user: UserInfo) => void;
  onUserLeave: (userId: string) => void;
  onCursorUpdate: (cursor: CursorState) => void;
  onRoomInfo: (info: RoomInfo) => void;
  onError: (code: string, msg: string) => void;
  onLog: (msg: string) => void;
}

/**
 * WebSocket client that speaks the RealtimeCollabEngine wire protocol.
 *
 * Lifecycle: connect → authenticate → join_room → operate / heartbeat / ...
 */
export class WsClient {
  private ws: WebSocket | null = null;
  private url: string;
  private userId: string;
  private token: string;
  private state: ConnectionState = 'disconnected';
  private heartbeatTimer: ReturnType<typeof setInterval> | null = null;
  private reconnectTimer: ReturnType<typeof setTimeout> | null = null;
  private reconnectDelay = 1000;
  private callbacks: WsCallbacks;

  constructor(
    url: string,
    userId: string,
    token: string,
    callbacks: WsCallbacks,
  ) {
    this.url = url;
    this.userId = userId;
    this.token = token;
    this.callbacks = callbacks;
  }

  getState() { return this.state; }

  connect() {
    if (this.state !== 'disconnected') return;
    this.setState('connecting');
    this.log(`Connecting to ${this.url}...`);

    try {
      this.ws = new WebSocket(this.url);
      this.ws.onopen = () => this.onOpen();
      this.ws.onmessage = (e) => this.onMessage(e.data);
      this.ws.onclose = (e) => this.onClose(e.reason);
      this.ws.onerror = () => this.log('WebSocket error');
    } catch (err: any) {
      this.log(`Connection failed: ${err.message}`);
      this.setState('disconnected');
      this.scheduleReconnect();
    }
  }

  disconnect() {
    this.clearTimers();
    if (this.ws) {
      try { this.ws.close(); } catch { /* ignore */ }
      this.ws = null;
    }
    this.setState('disconnected');
  }

  joinRoom(roomId: string) { this.send(Msg.joinRoom(roomId)); }
  leaveRoom(roomId: string) { this.send(Msg.leaveRoom(roomId)); }
  sendOperation(op: EditOp) { this.send(Msg.operation(op)); }
  sendCursor(cursor: CursorState) { this.send(Msg.cursorUpdate(cursor)); }
  sendHeartbeat() { this.send(Msg.heartbeat()); }

  private send(msg: WireMessage) {
    if (this.ws?.readyState === WebSocket.OPEN) {
      this.ws.send(JSON.stringify(msg));
    }
  }

  // ---- lifecycle -----------------------------------------------------------

  private onOpen() {
    this.setState('connected');
    this.log('WebSocket connected, authenticating...');
    this.send(Msg.connect(this.userId, this.token));
    this.startHeartbeat();
  }

  private onClose(reason: string) {
    this.clearTimers();
    this.setState('disconnected');
    this.log(`Disconnected${reason ? ': ' + reason : ''}`);
    this.scheduleReconnect();
  }

  private onMessage(raw: string) {
    let msg: WireMessage;
    try { msg = JSON.parse(raw); } catch {
      this.log('Failed to parse message');
      return;
    }
    this.dispatch(msg);
  }

  // ---- dispatch -------------------------------------------------------------

  private dispatch(msg: WireMessage) {
    const p = msg.payload ?? {};
    switch (msg.type) {
      case MessageType.CONNECT: {
        const status = p.status as string;
        if (status === 'connected' || status === 'authenticated') {
          this.setState('authenticated');
          this.log(`Authenticated as ${this.userId}`);
        }
        break;
      }
      case MessageType.OPERATION:
        this.callbacks.onRemoteOp({
          opId: (p.opId ?? '') as string,
          userId: (p.userId ?? '') as string,
          version: (p.version ?? 0) as number,
          type: (p.type ?? 'retain') as any,
          position: (p.position ?? 0) as number,
          content: (p.content ?? '') as string,
        });
        break;
      case MessageType.OPERATION_ACK:
        this.callbacks.onAck((p.opId ?? '') as string, (p.version ?? 0) as number);
        break;
      case MessageType.USER_JOIN:
        this.callbacks.onUserJoin({
          userId: (p.userId ?? '') as string,
          username: (p.username ?? 'unknown') as string,
          online: true,
        });
        break;
      case MessageType.USER_LEAVE:
        this.callbacks.onUserLeave((p.userId ?? '') as string);
        break;
      case MessageType.CURSOR_UPDATE:
        this.callbacks.onCursorUpdate({
          userId: (p.userId ?? '') as string,
          position: (p.position ?? 0) as number,
          selectionStart: (p.selectionStart ?? 0) as number,
          selectionEnd: (p.selectionEnd ?? 0) as number,
        });
        break;
      case MessageType.ROOM_INFO:
        this.callbacks.onRoomInfo({
          roomId: (p.roomId ?? '') as string,
          documentId: (p.documentId ?? '') as string,
          userCount: (p.userCount ?? 0) as number,
          version: (p.version ?? 0) as number,
        });
        break;
      case MessageType.ERROR:
        this.callbacks.onError((p.code ?? 'UNKNOWN') as string, (p.message ?? '') as string);
        break;
      case MessageType.HEARTBEAT:
        if (p.ping) this.send(Msg.heartbeatResp());
        break;
    }
  }

  // ---- helpers --------------------------------------------------------------

  private setState(s: ConnectionState) {
    this.state = s;
    this.callbacks.onStateChange(s);
  }

  private log(msg: string) { this.callbacks.onLog(msg); }

  private startHeartbeat() {
    this.heartbeatTimer = setInterval(() => this.sendHeartbeat(), 25_000);
  }

  private clearTimers() {
    if (this.heartbeatTimer) { clearInterval(this.heartbeatTimer); this.heartbeatTimer = null; }
    if (this.reconnectTimer) { clearTimeout(this.reconnectTimer); this.reconnectTimer = null; }
  }

  private scheduleReconnect() {
    this.reconnectTimer = setTimeout(() => {
      this.reconnectDelay = Math.min(this.reconnectDelay * 2, 30_000);
      this.log(`Reconnecting in ${this.reconnectDelay / 1000}s...`);
      this.connect();
    }, this.reconnectDelay);
  }
}
