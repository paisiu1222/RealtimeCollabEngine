/**
 * 内置模拟服务器 — 支持多文档、多用户协作
 */
import { MessageType, Msg, type WireMessage, type EditOp } from './protocol';

interface RemoteConn {
  send(msg: string): void;
  userId: string;
  username: string;
  roomId: string;
}

export class MockServer {
  private docs = new Map<string, string>();   // docId → content
  private versions = new Map<string, number>(); // docId → version
  private conns: RemoteConn[] = [];

  createDoc(id: string, content: string) {
    this.docs.set(id, content);
    this.versions.set(id, 0);
  }
  deleteDoc(id: string) {
    this.docs.delete(id);
    this.versions.delete(id);
  }
  getDocContent(id: string): string {
    return this.docs.get(id) ?? '';
  }

  /** 直接设置文档内容（用于切换文档时保存） */
  setDocContent(id: string, content: string) {
    this.docs.set(id, content);
  }

  connect(conn: RemoteConn) {
    this.conns.push(conn);
    conn.send(JSON.stringify({
      type: MessageType.CONNECT,
      messageId: 'srv-connect',
      timestamp: new Date().toISOString(),
      payload: { status: 'authenticated', connectionId: conn.userId, username: conn.username },
    }));
  }

  disconnect(conn: RemoteConn) {
    this.conns = this.conns.filter(c => c !== conn);
    if (conn.roomId) {
      const msg = { type: MessageType.USER_LEAVE, messageId: 'srv-ul', timestamp: new Date().toISOString(), payload: { userId: conn.userId } };
      this.broadcast(conn.roomId, msg, conn.userId);
    }
  }

  handle(conn: RemoteConn, raw: string) {
    let msg: WireMessage;
    try { msg = JSON.parse(raw); } catch { return; }

    switch (msg.type) {
      case MessageType.JOIN_ROOM: {
        const roomId = (msg.payload.roomId as string) || 'default';
        // 离开旧房间
        if (conn.roomId && conn.roomId !== roomId) {
          this.handle(conn, JSON.stringify({ type: MessageType.LEAVE_ROOM, messageId: 'auto-lr', timestamp: new Date().toISOString(), payload: { roomId: conn.roomId } }));
        }
        conn.roomId = roomId;

        // 告知当前房间所有用户
        this.broadcast(roomId, {
          type: MessageType.USER_JOIN,
          messageId: `srv-uj-${conn.userId}`,
          timestamp: new Date().toISOString(),
          payload: { userId: conn.userId, username: conn.username },
        }, conn.userId);

        // 发送已有用户列表
        for (const c of this.conns) {
          if (c.roomId === roomId && c.userId !== conn.userId) {
            conn.send(JSON.stringify({
              type: MessageType.USER_JOIN,
              messageId: `srv-eu-${c.userId}`,
              timestamp: new Date().toISOString(),
              payload: { userId: c.userId, username: c.username },
            }));
          }
        }

        // 发送房间信息
        const count = this.conns.filter(c => c.roomId === roomId).length;
        conn.send(JSON.stringify({
          type: MessageType.ROOM_INFO,
          messageId: 'srv-ri',
          timestamp: new Date().toISOString(),
          payload: { roomId, userCount: count, version: this.versions.get(roomId) ?? 0, documentId: roomId },
        }));

        // 同步文档内容
        conn.send(JSON.stringify({
          type: MessageType.DOCUMENT_SYNC,
          messageId: 'srv-ds',
          timestamp: new Date().toISOString(),
          payload: { content: this.docs.get(roomId) ?? '', version: this.versions.get(roomId) ?? 0 },
        }));
        break;
      }

      case MessageType.LEAVE_ROOM: {
        const roomId = (msg.payload.roomId as string) || conn.roomId;
        if (roomId) {
          this.broadcast(roomId, {
            type: MessageType.USER_LEAVE,
            messageId: `srv-ul-${conn.userId}`,
            timestamp: new Date().toISOString(),
            payload: { userId: conn.userId },
          }, conn.userId);
          conn.roomId = '';
        }
        break;
      }

      case MessageType.OPERATION: {
        if (!conn.roomId) return;
        const p = msg.payload;
        const opType = ((p as any).operation?.type as string) || (p as any).type as string || 'insert';
        const opPosition = ((p as any).operation?.position as number) ?? (p as any).position as number ?? 0;
        const opContent = ((p as any).operation?.content as string) ?? (p as any).content as string ?? '';

        const newVer = (this.versions.get(conn.roomId) ?? 0) + 1;
        this.versions.set(conn.roomId, newVer);

        // 应用到服务器文档
        let doc = this.docs.get(conn.roomId) ?? '';
        switch (opType) {
          case 'insert':
            doc = doc.slice(0, opPosition) + opContent + doc.slice(opPosition);
            break;
          case 'delete': {
            const end = opPosition + opContent.length;
            doc = doc.slice(0, opPosition) + doc.slice(end);
            break;
          }
          case 'replace': {
            const end = opPosition + opContent.length;
            doc = doc.slice(0, opPosition) + opContent + doc.slice(end);
            break;
          }
        }
        this.docs.set(conn.roomId, doc);

        // ACK
        conn.send(JSON.stringify({
          type: MessageType.OPERATION_ACK,
          messageId: `srv-ack-${(p.opId as string) ?? ''}`,
          timestamp: new Date().toISOString(),
          payload: { opId: (p.opId as string) ?? '', version: newVer, status: 'success' },
        }));

        // 广播给其他人
        this.broadcast(conn.roomId, {
          type: MessageType.OPERATION,
          messageId: `srv-op`,
          timestamp: new Date().toISOString(),
          payload: {
            opId: (p.opId as string) ?? '',
            userId: (p.userId as string) ?? conn.userId,
            version: newVer,
            type: opType,
            position: opPosition,
            content: opContent,
          },
        }, conn.userId);
        break;
      }

      case MessageType.CURSOR_UPDATE:
        if (conn.roomId) this.broadcast(conn.roomId, msg, conn.userId);
        break;

      case MessageType.HEARTBEAT:
        conn.send(JSON.stringify(Msg.heartbeatResp()));
        break;
    }
  }

  private broadcast(roomId: string, msg: any, excludeUserId?: string) {
    const raw = JSON.stringify(msg);
    for (const c of this.conns) {
      if (c.roomId === roomId && c.userId !== excludeUserId) {
        try { c.send(raw); } catch { /* ignore */ }
      }
    }
  }
}
