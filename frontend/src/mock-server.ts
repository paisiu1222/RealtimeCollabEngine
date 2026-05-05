/**
 * 内置模拟服务器 — BroadcastChannel 实现跨标签页实时协作
 */
import { MessageType, Msg, type WireMessage } from './protocol';

interface ConnHandler {
  send(msg: string): void;
  userId: string;
  username: string;
  roomId: string;
}

const CH = new BroadcastChannel('rce-collab');
const LS_DOCS = 'rce_docs';
const LS_VERSIONS = 'rce_versions';

export class MockServer {
  private docs = new Map<string, string>();
  private versions = new Map<string, number>();
  handler: ConnHandler | null = null;
  myUserId = '';

  constructor() {
    // 从 localStorage 恢复文档
    try {
      const d = JSON.parse(localStorage.getItem(LS_DOCS) || '{}');
      const v = JSON.parse(localStorage.getItem(LS_VERSIONS) || '{}');
      for (const [k, val] of Object.entries(d)) { this.docs.set(k, val as string); }
      for (const [k, val] of Object.entries(v)) { this.versions.set(k, val as number); }
    } catch { /* ignore */ }
  }

  connect(conn: ConnHandler) {
    this.handler = conn;
    this.myUserId = conn.userId;

    CH.onmessage = (e: MessageEvent) => {
      const env = e.data as { from: string; msg: WireMessage };
      if (!env || env.from === this.myUserId) return;
      const m = env.msg, p = m.payload as any;

      switch (m.type) {
        case MessageType.OPERATION: {
          const rid = p.roomId as string;
          if (!rid) break;
          const opType = p.type || p.operation?.type || 'insert';
          const opPos = p.position ?? p.operation?.position ?? 0;
          const opContent = p.content ?? p.operation?.content ?? '';
          const ver = (this.versions.get(rid) ?? 0) + 1;
          this.versions.set(rid, ver);
          let doc = this.docs.get(rid) ?? '';
          switch (opType) {
            case 'insert': doc = doc.slice(0, opPos) + opContent + doc.slice(opPos); break;
            case 'delete': doc = doc.slice(0, opPos) + doc.slice(opPos + opContent.length); break;
            case 'replace': doc = doc.slice(0, opPos) + opContent + doc.slice(opPos + opContent.length); break;
          }
          this.docs.set(rid, doc);
          this.saveDocs();
          break;
        }
        case MessageType.USER_JOIN: {
          // 如果本地在同一房间，回复自己的信息 + 完整文档内容
          if (this.handler?.roomId && this.handler?.roomId === p.roomId) {
            const rid = this.handler.roomId;
            this.channelSend({
              type: MessageType.USER_JOIN, messageId: 'srv-uj-reply', timestamp: new Date().toISOString(),
              payload: { userId: this.myUserId, username: this.handler.username, roomId: rid },
            });
            // 发送当前文档内容给新加入的用户
            this.channelSend({
              type: MessageType.DOCUMENT_SYNC, messageId: 'srv-ds-reply', timestamp: new Date().toISOString(),
              payload: { content: this.docs.get(rid) ?? '', version: this.versions.get(rid) ?? 0 },
            });
          }
          break;
        }
      }

      // 转发给本地 app（用于 UI 更新）
      this.handler?.send(JSON.stringify(m));
    };

    conn.send(JSON.stringify({
      type: MessageType.CONNECT, messageId: 'srv-connect',
      timestamp: new Date().toISOString(),
      payload: { status: 'authenticated', connectionId: conn.userId, username: conn.username },
    }));
  }

  disconnect() {
    if (this.handler?.roomId) {
      this.channelSend({
        type: MessageType.USER_LEAVE, messageId: `srv-ul-${this.myUserId}`,
        timestamp: new Date().toISOString(), payload: { userId: this.myUserId },
      });
    }
  }

  createDoc(id: string, content: string) { this.docs.set(id, content); this.versions.set(id, 0); this.saveDocs(); }
  deleteDoc(id: string) { this.docs.delete(id); this.versions.delete(id); this.saveDocs(); }
  getDocContent(id: string): string { return this.docs.get(id) ?? ''; }
  setDocContent(id: string, content: string) { this.docs.set(id, content); this.saveDocs(); }

  handle(conn: ConnHandler, raw: string) {
    let msg: WireMessage;
    try { msg = JSON.parse(raw); } catch { return; }
    const p = msg.payload as any;

    switch (msg.type) {
      case MessageType.JOIN_ROOM: {
        const roomId = (p.roomId as string) || 'default';
        if (conn.roomId && conn.roomId !== roomId) {
          this.channelSend({ type: MessageType.USER_LEAVE, messageId: `srv-ul-${conn.userId}`, timestamp: new Date().toISOString(), payload: { userId: conn.userId } });
        }
        conn.roomId = roomId;
        if (!this.docs.has(roomId)) { this.docs.set(roomId, ''); this.versions.set(roomId, 0); }

        // 广播加入
        this.channelSend({ type: MessageType.USER_JOIN, messageId: `srv-uj-${conn.userId}`, timestamp: new Date().toISOString(), payload: { userId: conn.userId, username: conn.username, roomId } });

        const ver = this.versions.get(roomId) ?? 0;
        conn.send(JSON.stringify({ type: MessageType.ROOM_INFO, messageId: 'srv-ri', timestamp: new Date().toISOString(), payload: { roomId, userCount: 1, version: ver, documentId: roomId } }));
        conn.send(JSON.stringify({ type: MessageType.DOCUMENT_SYNC, messageId: 'srv-ds', timestamp: new Date().toISOString(), payload: { content: this.docs.get(roomId) ?? '', version: ver } }));
        break;
      }

      case MessageType.LEAVE_ROOM: {
        const lrRoom = (p.roomId as string) || conn.roomId;
        if (lrRoom) {
          this.channelSend({ type: MessageType.USER_LEAVE, messageId: `srv-ul-${conn.userId}`, timestamp: new Date().toISOString(), payload: { userId: conn.userId } });
          conn.roomId = '';
        }
        break;
      }

      case MessageType.OPERATION: {
        if (!conn.roomId) return;
        const opType = p.type || p.operation?.type || 'insert';
        const opPos = p.position ?? p.operation?.position ?? 0;
        const opContent = p.content ?? p.operation?.content ?? '';

        const newVer = (this.versions.get(conn.roomId) ?? 0) + 1;
        this.versions.set(conn.roomId, newVer);
        let doc = this.docs.get(conn.roomId) ?? '';
        switch (opType) {
          case 'insert': doc = doc.slice(0, opPos) + opContent + doc.slice(opPos); break;
          case 'delete': doc = doc.slice(0, opPos) + doc.slice(opPos + opContent.length); break;
          case 'replace': doc = doc.slice(0, opPos) + opContent + doc.slice(opPos + opContent.length); break;
        }
        this.docs.set(conn.roomId, doc);
        this.saveDocs();

        conn.send(JSON.stringify({ type: MessageType.OPERATION_ACK, messageId: 'srv-ack', timestamp: new Date().toISOString(), payload: { opId: (p.opId as string) ?? '', version: newVer, status: 'success' } }));
        this.channelSend({ type: MessageType.OPERATION, messageId: 'srv-op', timestamp: new Date().toISOString(), payload: { roomId: conn.roomId, opId: (p.opId as string) ?? '', userId: this.myUserId, version: newVer, type: opType, position: opPos, content: opContent } });
        break;
      }

      case MessageType.CURSOR_UPDATE:
        if (conn.roomId) this.channelSend(msg);
        break;

      case MessageType.HEARTBEAT:
        conn.send(JSON.stringify(Msg.heartbeatResp()));
        break;
    }
  }

  private channelSend(msg: any) { CH.postMessage({ from: this.myUserId, msg }); }
  private saveDocs() {
    try {
      localStorage.setItem(LS_DOCS, JSON.stringify(Object.fromEntries(this.docs)));
      localStorage.setItem(LS_VERSIONS, JSON.stringify(Object.fromEntries(this.versions)));
    } catch { /* quota exceeded */ }
  }
}
