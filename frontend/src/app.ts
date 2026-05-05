import { WsClient, type ConnectionState } from './ws-client';
import { OTEngine } from './ot-engine';
import { MockServer } from './mock-server';

import type { EditOp, CursorState, UserInfo, WireMessage } from './protocol';

// ==================== 状态 ====================
let ws: WsClient;
let ot = new OTEngine();
let mock: MockServer | null = null;
let mockConn: { userId: string; username: string; roomId: string; send(raw: string): void } | null = null;

const USE_MOCK = location.hash === '#mock' || !location.hostname.includes('collab');
const WS_URL = USE_MOCK ? 'mock://local'
  : (location.protocol === 'https:' ? 'wss://' : 'ws://') + location.host + '/ws';

let userId = '';
let token = '';
let username = '';

interface DocMeta { id: string; title: string; ownerId: string; updatedAt: string; version: number; }
const docList = new Map<string, DocMeta>();
let activeDocId = '';
const remoteUsers = new Map<string, { el: HTMLElement; color: string }>();
const remoteCursors = new Map<string, HTMLElement>();
let editorEl!: HTMLElement;
let saveTimer: ReturnType<typeof setTimeout> | null = null;
let applyingRemote = false;

// ==================== 初始化 ====================
export function init(userName: string) {
  username = userName || '用户-' + Math.random().toString(36).slice(2, 6);
  userId = 'user-' + username.toLowerCase().replace(/[^a-z0-9]/g, '');
  token = 'tok-' + Math.random().toString(36).slice(2, 14);

  ot.setUserId(userId);

  editorEl = document.getElementById('editor')!;
  // IME 组合输入期间不触发同步
  let composing = false;
  editorEl.addEventListener('compositionstart', () => { composing = true; });
  editorEl.addEventListener('compositionend', () => { composing = false; onEditorInput(); });
  editorEl.addEventListener('input', () => {
    if (!composing && !applyingRemote) onEditorInput();
  });
  // Enter 键插入 \n（用 Selection API，兼容所有浏览器）
  editorEl.addEventListener('keydown', (e) => {
    if (e.key === 'Enter' && !composing) {
      e.preventDefault();
      const sel = window.getSelection();
      if (sel && sel.rangeCount > 0) {
        const range = sel.getRangeAt(0);
        range.deleteContents();
        const textNode = document.createTextNode('\n');
        range.insertNode(textNode);
        range.setStartAfter(textNode);
        range.collapse(true);
        sel.removeAllRanges();
        sel.addRange(range);
        onEditorInput();
      }
    }
  });
  editorEl.addEventListener('keyup', onCursorMove);
  editorEl.addEventListener('mouseup', onCursorMove);
  editorEl.addEventListener('click', onCursorMove);

  document.getElementById('newDocBtn')!.addEventListener('click', () => createDocument());
  document.getElementById('docTitle')!.addEventListener('input', onTitleChange);
  document.getElementById('searchInput')!.addEventListener('input', renderDocList);

  document.getElementById('localName')!.textContent = username;
  document.getElementById('localAvatar')!.style.background = userColor(userId);

  if (USE_MOCK) {
    initMock();
  } else {
    ws = new WsClient(WS_URL, userId, token, makeCallbacks());
    ws.connect();
  }
  addLog('就绪 — ' + (USE_MOCK ? '模拟模式' : '在线模式'));
}

// ==================== 模拟模式 ====================
function initMock() {
  mock = new MockServer();
  mockConn = {
    userId, username, roomId: '',
    send(raw: string) { dispatchFromServer(raw); },
  };
  mock.connect(mockConn);
  setConnectionState('authenticated');
  addLog('模拟服务器已连接');

  // 关闭/刷新页面前通知其他标签页
  window.addEventListener('beforeunload', () => {
    if (mock && mockConn && activeDocId) {
      mock.handle(mockConn, JSON.stringify({ type: 'leave_room', messageId: 'bye', timestamp: new Date().toISOString(), payload: { roomId: activeDocId } }));
    }
  });

  // 预置两个示例文档
  seedMockDocs();
}

function seedMockDocs() {
  const docs = [
    { id: 'doc-001', title: '项目需求文档', content: '# 项目需求文档\n\n## 1. 背景\n本项目旨在构建一个高性能的实时协作编辑引擎...\n\n## 2. 核心功能\n- 多人实时协同编辑\n- 操作变换 (OT) 冲突解决\n- 文档快照与恢复\n- 离线队列同步\n\n## 3. 技术栈\n- 后端: C++14 + Boost + SQLite3\n- 前端: TypeScript + Vite\n- 协议: WebSocket + JSON\n- 算法: Operation Transformation\n\n欢迎协作编辑！' },
    { id: 'doc-002', title: '会议记录 — 2026-05-03', content: '# 会议记录\n\n**日期**: 2026年5月3日\n**参会人**: 张三、李四、王五\n\n## 议题\n1. 本周迭代进度回顾\n2. OT 算法性能优化方案\n3. 部署上线时间确认\n\n## 决议\n- OT 策略模式重构已完成，通过所有 116 个测试\n- 前端采用 Vite + TypeScript 重构\n- 计划下周一灰度上线\n\n---\n*请在此补充您的笔记...*' },
  ];
  docs.forEach(d => {
    docList.set(d.id, {
      id: d.id, title: d.title, ownerId: 'system',
      updatedAt: new Date().toLocaleString('zh-CN'), version: 0,
    });
    mock!.createDoc(d.id, d.content);

  });
  renderDocList();
}

// ==================== 服务器消息分发 ====================
function dispatchFromServer(raw: string) {
  let msg: WireMessage;
  try { msg = JSON.parse(raw); } catch { return; }
  const p = msg.payload ?? {};
  switch (msg.type) {
    case 'connect':
      if (p.status === 'authenticated') { setConnectionState('authenticated'); addLog('已认证'); }
      break;
    case 'join_room':
      addLog('已加入房间');
      break;
    case 'room_info':
      break;
    case 'document_sync': {
      const c = (p.content ?? '') as string;
      const v = (p.version ?? 0) as number;
      ot.reset(c, v);
      editorEl.textContent = c;
      if (!activeDocId) addLog('文档已加载');
      break;
    }
    case 'user_join':
      addRemoteUser({ userId: p.userId as string, username: p.username as string, online: true });
      break;
    case 'user_leave':
      removeRemoteUser(p.userId as string);
      break;
    case 'operation': {
      const op: EditOp = {
        opId: (p.opId ?? '') as string,
        userId: (p.userId ?? '') as string,
        version: (p.version ?? 0) as number,
        type: ((p as any).type ?? 'insert') as any,
        position: ((p as any).position ?? 0) as number,
        content: ((p as any).content ?? '') as string,
      };
      if (op.userId !== userId) applyRemoteOp(op);
      break;
    }
    case 'operation_ack':
      ot.ack(p.opId as string, p.version as number);
      markSaved();
      break;
    case 'cursor_update':
      updateRemoteCursor({
        userId: p.userId as string,
        position: p.position as number,
        selectionStart: (p.selectionStart ?? 0) as number,
        selectionEnd: (p.selectionEnd ?? 0) as number,
      });
      break;
    case 'heartbeat':
      if (p.ping && mock) {
        const conn = { userId, username, roomId: activeDocId ?? 'demo', send(_: string) {} };
        mock.handle(conn, JSON.stringify({ type: 'heartbeat', messageId: 'hb', timestamp: new Date().toISOString(), payload: { pong: true } }));
      }
      break;
    case 'error':
      addLog(`❌ ${p.code}: ${p.message}`);
      break;
  }
}

function makeCallbacks() {
  return {
    onStateChange: (s: ConnectionState) => setConnectionState(s),
    onRemoteOp: (op: EditOp) => { if (op.userId !== userId) applyRemoteOp(op); },
    onAck: (opId: string, version: number) => { ot.ack(opId, version); markSaved(); },
    onUserJoin: (u: UserInfo) => addRemoteUser(u),
    onUserLeave: (id: string) => removeRemoteUser(id),
    onCursorUpdate: (c: CursorState) => updateRemoteCursor(c),
    onRoomInfo: () => {},
    onError: (code: string, msg: string) => addLog(`❌ ${code}: ${msg}`),
    onLog: (msg: string) => addLog(msg),
  };
}

// ==================== 编辑器事件 ====================
function onEditorInput() {
  const text = editorEl.textContent ?? '';
  const ops = ot.diff(text);
  if (ops.length > 0) {
    markSyncing();
    for (const op of ops) {
      if (mock && mockConn) {
        // Route through mock.handle() so it processes the op,
        // increments version, and sends back operation_ack
        mock.handle(mockConn, JSON.stringify({
          type: 'operation', messageId: op.opId, timestamp: new Date().toISOString(),
          payload: { opId: op.opId, userId, version: ot.getVersion() + 1,
            type: op.type, position: op.position, content: op.content },
        }));
      } else if (ws) {
        ws.sendOperation(op);
      }
    }
    updateDocMeta();
  }
}

function applyRemoteOp(op: EditOp) {
  const transformed = ot.transformRemote(op);
  const text = editorEl.textContent ?? '';
  applyingRemote = true;
  switch (transformed.type) {
    case 'insert':
      editorEl.textContent = text.slice(0, transformed.position) + transformed.content + text.slice(transformed.position);
      break;
    case 'delete': {
      const e = transformed.position + transformed.content.length;
      editorEl.textContent = text.slice(0, transformed.position) + text.slice(e);
      break;
    }
    case 'replace': {
      const e = transformed.position + transformed.content.length;
      editorEl.textContent = text.slice(0, transformed.position) + transformed.content + text.slice(e);
      break;
    }
  }
  applyingRemote = false;
  updateDocMeta();
}

function onTitleChange() {
  const title = (document.getElementById('docTitle') as HTMLInputElement).value.trim();
  if (activeDocId && docList.has(activeDocId)) {
    docList.get(activeDocId)!.title = title || '无标题文档';
    renderDocList();
  }
}

function onCursorMove() {
  const sel = window.getSelection();
  if (!sel || !editorEl.contains(sel.anchorNode)) return;
  const pos = getCursorTextOffset(editorEl, sel.anchorNode, sel.anchorOffset);
  const cursor: CursorState = { userId, position: pos, selectionStart: pos, selectionEnd: pos + (sel.isCollapsed ? 0 : sel.toString().length) };
  if (mock) {
    dispatchFromServer(JSON.stringify({ type: 'cursor_update', messageId: 'cur', timestamp: new Date().toISOString(), payload: { ...cursor } }));
  } else if (ws) {
    ws.sendCursor(cursor);
  }
}

// ==================== 文档管理 ====================
function createDocument() {
  const id = 'doc-' + Date.now();
  const doc: DocMeta = {
    id, title: '无标题文档',
    ownerId: userId, updatedAt: new Date().toLocaleString('zh-CN'), version: 0,
  };
  docList.set(id, doc);
  if (mock) mock.createDoc(id, '');
  renderDocList();
  openDocument(id);
}

function openDocument(id: string) {
  // 保存并离开当前文档
  if (activeDocId) {
    const oldTitle = (document.getElementById('docTitle') as HTMLInputElement).value.trim();
    if (oldTitle && docList.has(activeDocId)) {
      docList.get(activeDocId)!.title = oldTitle || '无标题文档';
    }
    const currentContent = editorEl.textContent ?? '';
    if (mock && mockConn) {
      mock.setDocContent(activeDocId, currentContent);
      mock.handle(mockConn, JSON.stringify({ type: 'leave_room', messageId: 'lr', timestamp: new Date().toISOString(), payload: { roomId: activeDocId } }));
    }
    // 自动快照

  }

  // 清理远程用户/光标
  remoteUsers.forEach((v, k) => { v.el.remove(); remoteCursors.get(k)?.remove(); });
  remoteUsers.clear();
  remoteCursors.clear();

  activeDocId = id;
  const doc = docList.get(id);
  if (!doc) return;

  // 切换 UI
  document.getElementById('emptyState')!.style.display = 'none';
  document.getElementById('editorWorkspace')!.style.display = 'flex';
  document.getElementById('collabPanel')!.style.display = 'flex';
  (document.getElementById('docTitle') as HTMLInputElement).value = doc.title;
  ot.reset('', 0);
  editorEl.textContent = '';

  // 加入房间并初始化版本管理
  if (mock && mockConn) {
    mock.handle(mockConn, JSON.stringify({ type: 'join_room', messageId: 'jr', timestamp: new Date().toISOString(), payload: { roomId: id } }));
    const syncContent = mock.getDocContent(id);
    ot.reset(syncContent, 0);
    editorEl.textContent = syncContent;

  }

  renderDocList();

  updateDocMeta();
  addLog(`已打开: ${doc.title}`);
}

function deleteDocument(id: string) {
  if (!confirm(`确定删除「${docList.get(id)?.title ?? id}」？`)) return;
  docList.delete(id);
  if (mock) mock.deleteDoc(id);
  if (activeDocId === id) {
    activeDocId = '';
    editorEl.textContent = '';
    document.getElementById('emptyState')!.style.display = 'flex';
    document.getElementById('editorWorkspace')!.style.display = 'none';
    document.getElementById('collabPanel')!.style.display = 'none';
  }
  renderDocList();
  addLog('文档已删除');
}

function renderDocList() {
  const container = document.getElementById('docList')!;
  const q = ((document.getElementById('searchInput') as HTMLInputElement).value || '').toLowerCase();
  const entries = [...docList.values()]
    .filter(d => !q || d.title.toLowerCase().includes(q))
    .sort((a, b) => (a.updatedAt < b.updatedAt ? 1 : -1));

  container.innerHTML = entries.length === 0
    ? `<div style="padding:24px;text-align:center;color:var(--text-dim);font-size:13px">${q ? '无匹配文档' : '暂无文档，点击「+ 新建」'}</div>`
    : entries.map(d => `
      <div class="doc-item${d.id === activeDocId ? ' active' : ''}" onclick="window._openDoc('${d.id}')">
        <div class="doc-item-icon">📄</div>
        <div class="doc-item-body">
          <div class="doc-item-title">${escapeHtml(d.title || '无标题文档')}</div>
          <div class="doc-item-meta">${d.updatedAt}</div>
        </div>
        <div class="doc-item-actions">
          <button onclick="event.stopPropagation();window._delDoc('${d.id}')" title="删除">🗑</button>
        </div>
      </div>
    `).join('');
}

// ==================== 远程用户/光标 ====================
function addRemoteUser(u: UserInfo) {
  if (remoteUsers.has(u.userId)) return;
  const color = userColor(u.userId);
  const li = document.createElement('li');
  li.className = 'collab-item';
  li.id = 'cu-' + u.userId;
  li.innerHTML = `<div class="collab-avatar" style="background:${color}">${(u.username || u.userId)[0].toUpperCase()}</div><div class="collab-info"><span class="collab-name">${escapeHtml(u.username || u.userId)}</span><span class="collab-role">协作者</span></div>`;
  document.getElementById('collabList')!.appendChild(li);
  remoteUsers.set(u.userId, { el: li, color });
  updateInfoPanel();
}

function removeRemoteUser(id: string) {
  remoteUsers.get(id)?.el.remove();
  remoteUsers.delete(id);
  const cursor = remoteCursors.get(id);
  if (cursor) { cursor.remove(); remoteCursors.delete(id); }
  updateInfoPanel();
}

function updateRemoteCursor(c: CursorState) {
  if (c.userId === userId) return;
  const color = remoteUsers.get(c.userId)?.color ?? userColor(c.userId);
  let cursor = remoteCursors.get(c.userId);
  if (!cursor) {
    cursor = document.createElement('div');
    cursor.className = 'remote-cursor';
    cursor.innerHTML = `<div class="remote-cursor-bar" style="background:${color}"></div><div class="remote-cursor-label" style="background:${color}">${c.userId.slice(-4)}</div>`;
    document.getElementById('cursorsLayer')!.appendChild(cursor);
    remoteCursors.set(c.userId, cursor);
  }
  // 估算光标位置
  const text = editorEl.textContent ?? '';
  const before = text.slice(0, c.position).split('\n');
  const top = (before.length - 1) * 27 + 2;
  const left = (before[before.length - 1]?.length ?? 0) * 9.6;
  cursor.style.top = top + 'px';
  cursor.style.left = left + 'px';
}

// ==================== UI 辅助 ====================
function setConnectionState(s: ConnectionState) {
  const dot = document.getElementById('statusDot')!;
  const text = document.getElementById('statusText')!;
  dot.className = 'status-dot';
  if (s === 'authenticated') { dot.classList.add('connected'); text.textContent = '已连接'; }
  else if (s === 'connected' || s === 'connecting') { dot.classList.add(s === 'connecting' ? 'connecting' : 'connected'); text.textContent = s === 'connecting' ? '连接中...' : '已连接'; }
  else { text.textContent = '未连接'; }
}

function markSyncing() {
  const el = document.getElementById('saveIndicator')!;
  el.textContent = '保存中...';
  el.className = 'save-indicator syncing';
  if (saveTimer) clearTimeout(saveTimer);
  saveTimer = setTimeout(() => markSaved(), 2000);
}
function markSaved() {
  const el = document.getElementById('saveIndicator')!;
  el.textContent = '已保存';
  el.className = 'save-indicator';
}
function updateDocMeta() {
  const doc = docList.get(activeDocId);
  if (!doc) return;
  doc.updatedAt = new Date().toLocaleString('zh-CN');
  document.getElementById('docMetaOwner')!.textContent = username;
  document.getElementById('docMetaTime')!.textContent = doc.updatedAt;
  updateInfoPanel();
  renderDocList();
}
function updateInfoPanel() {
  document.getElementById('infoChars')!.textContent = String((editorEl.textContent ?? '').length);
  document.getElementById('infoOnline')!.textContent = String(remoteUsers.size + 1);
}
function addLog(msg: string) {
  const c = document.getElementById('logContainer')!;
  const d = document.createElement('div');
  d.className = 'log-entry';
  d.textContent = `[${new Date().toLocaleTimeString('zh-CN')}] ${msg}`;
  c.appendChild(d);
  while (c.children.length > 100) c.firstChild?.remove();
  c.scrollTop = c.scrollHeight;
}

// ==================== 工具函数 ====================
function getCursorTextOffset(container: Node, node: Node | null, offset: number): number {
  if (!node || !container.contains(node)) return 0;
  const walker = document.createTreeWalker(container, NodeFilter.SHOW_TEXT);
  let pos = 0;
  let current: Text | null;
  while ((current = walker.nextNode() as Text | null)) {
    if (current === node) return pos + offset;
    pos += current.textContent?.length ?? 0;
  }
  return pos;
}
function userColor(id: string): string {
  const colors = ['#6366f1','#ec4899','#14b8a6','#f97316','#8b5cf6','#06b6d4','#ef4444','#22c55e'];
  return colors[Math.abs(hash(id)) % colors.length];
}
function hash(s: string): number { let h = 0; for (let i = 0; i < s.length; i++) h = (h * 31 + s.charCodeAt(i)) | 0; return h; }
function escapeHtml(s: string): string {
  const d = document.createElement('div');
  d.textContent = s;
  return d.innerHTML;
}

// 暴露给 HTML onclick
(window as any)._openDoc = openDocument;
(window as any)._delDoc = deleteDocument;
