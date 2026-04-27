-- RealtimeCollabEngine 数据库初始化脚本
-- SQLite3 Schema Definition

-- ============================================
-- 1. 用户表 (users)
-- ============================================
CREATE TABLE IF NOT EXISTS users (
    user_id TEXT PRIMARY KEY,
    username TEXT UNIQUE NOT NULL,
    email TEXT UNIQUE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    last_login TIMESTAMP,
    is_active INTEGER DEFAULT 1
);

CREATE INDEX IF NOT EXISTS idx_users_username ON users(username);
CREATE INDEX IF NOT EXISTS idx_users_email ON users(email);

-- ============================================
-- 2. 会话表 (sessions)
-- ============================================
CREATE TABLE IF NOT EXISTS sessions (
    session_id TEXT PRIMARY KEY,
    user_id TEXT NOT NULL,
    token TEXT UNIQUE NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    expires_at TIMESTAMP NOT NULL,
    is_valid INTEGER DEFAULT 1,
    FOREIGN KEY(user_id) REFERENCES users(user_id) ON DELETE CASCADE
);

CREATE INDEX IF NOT EXISTS idx_sessions_token ON sessions(token);
CREATE INDEX IF NOT EXISTS idx_sessions_user ON sessions(user_id);
CREATE INDEX IF NOT EXISTS idx_sessions_expires ON sessions(expires_at);

-- ============================================
-- 3. 文档表 (documents)
-- ============================================
CREATE TABLE IF NOT EXISTS documents (
    doc_id TEXT PRIMARY KEY,
    title TEXT NOT NULL DEFAULT 'Untitled Document',
    owner_id TEXT NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    current_version INTEGER DEFAULT 0,
    content TEXT DEFAULT '',
    is_deleted INTEGER DEFAULT 0,
    FOREIGN KEY(owner_id) REFERENCES users(user_id) ON DELETE CASCADE
);

CREATE INDEX IF NOT EXISTS idx_documents_owner ON documents(owner_id);
CREATE INDEX IF NOT EXISTS idx_documents_updated ON documents(updated_at DESC);

-- ============================================
-- 4. 文档协作者表 (document_collaborators)
-- ============================================
CREATE TABLE IF NOT EXISTS document_collaborators (
    doc_id TEXT NOT NULL,
    user_id TEXT NOT NULL,
    permission TEXT NOT NULL DEFAULT 'read', -- read, write, admin
    joined_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY(doc_id, user_id),
    FOREIGN KEY(doc_id) REFERENCES documents(doc_id) ON DELETE CASCADE,
    FOREIGN KEY(user_id) REFERENCES users(user_id) ON DELETE CASCADE
);

CREATE INDEX IF NOT EXISTS idx_collaborators_user ON document_collaborators(user_id);

-- ============================================
-- 5. 操作记录表 (operations)
-- ============================================
CREATE TABLE IF NOT EXISTS operations (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    doc_id TEXT NOT NULL,
    op_id TEXT UNIQUE NOT NULL,
    user_id TEXT NOT NULL,
    version INTEGER NOT NULL,
    op_type TEXT NOT NULL, -- insert, delete, replace
    position INTEGER,
    length INTEGER DEFAULT 0,
    content TEXT,
    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    applied INTEGER DEFAULT 0,
    FOREIGN KEY(doc_id) REFERENCES documents(doc_id) ON DELETE CASCADE,
    FOREIGN KEY(user_id) REFERENCES users(user_id) ON DELETE CASCADE
);

CREATE INDEX IF NOT EXISTS idx_operations_doc_version ON operations(doc_id, version);
CREATE INDEX IF NOT EXISTS idx_operations_op_id ON operations(op_id);
CREATE INDEX IF NOT EXISTS idx_operations_timestamp ON operations(timestamp DESC);

-- ============================================
-- 6. 快照表 (snapshots) - 用于文档状态恢复
-- ============================================
CREATE TABLE IF NOT EXISTS snapshots (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    doc_id TEXT NOT NULL,
    version INTEGER NOT NULL,
    content TEXT NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY(doc_id) REFERENCES documents(doc_id) ON DELETE CASCADE
);

CREATE INDEX IF NOT EXISTS idx_snapshots_doc_version ON snapshots(doc_id, version);
CREATE INDEX IF NOT EXISTS idx_snapshots_created ON snapshots(created_at DESC);

-- ============================================
-- 7. 房间表 (rooms) - 用于实时协作会话
-- ============================================
CREATE TABLE IF NOT EXISTS rooms (
    room_id TEXT PRIMARY KEY,
    doc_id TEXT NOT NULL UNIQUE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    max_users INTEGER DEFAULT 50,
    is_active INTEGER DEFAULT 1,
    FOREIGN KEY(doc_id) REFERENCES documents(doc_id) ON DELETE CASCADE
);

CREATE INDEX IF NOT EXISTS idx_rooms_doc ON rooms(doc_id);

-- ============================================
-- 8. 房间成员表 (room_members)
-- ============================================
CREATE TABLE IF NOT EXISTS room_members (
    room_id TEXT NOT NULL,
    user_id TEXT NOT NULL,
    joined_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    last_heartbeat TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    is_online INTEGER DEFAULT 1,
    PRIMARY KEY(room_id, user_id),
    FOREIGN KEY(room_id) REFERENCES rooms(room_id) ON DELETE CASCADE,
    FOREIGN KEY(user_id) REFERENCES users(user_id) ON DELETE CASCADE
);

CREATE INDEX IF NOT EXISTS idx_room_members_user ON room_members(user_id);

-- ============================================
-- 9. 系统配置表 (system_config)
-- ============================================
CREATE TABLE IF NOT EXISTS system_config (
    key TEXT PRIMARY KEY,
    value TEXT NOT NULL,
    description TEXT,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- ============================================
-- 插入默认配置
-- ============================================
INSERT OR IGNORE INTO system_config (key, value, description) VALUES
    ('max_document_size', '10485760', 'Maximum document size in bytes (10MB)'),
    ('session_timeout_minutes', '30', 'Session timeout in minutes'),
    ('heartbeat_interval_seconds', '30', 'Heartbeat interval in seconds'),
    ('max_operations_batch', '100', 'Maximum operations per batch'),
    ('version_history_limit', '1000', 'Maximum version history to keep');

-- ============================================
-- 视图 (Views)
-- ============================================

-- 活跃文档视图
DROP VIEW IF EXISTS active_documents;
CREATE VIEW active_documents AS
SELECT 
    d.doc_id,
    d.title,
    d.owner_id,
    u.username as owner_name,
    d.created_at,
    d.updated_at,
    d.current_version,
    COUNT(DISTINCT dc.user_id) as collaborator_count
FROM documents d
JOIN users u ON d.owner_id = u.user_id
LEFT JOIN document_collaborators dc ON d.doc_id = dc.doc_id
WHERE d.is_deleted = 0
GROUP BY d.doc_id;

-- 活跃房间视图
DROP VIEW IF EXISTS active_rooms;
CREATE VIEW active_rooms AS
SELECT 
    r.room_id,
    r.doc_id,
    d.title as document_title,
    COUNT(DISTINCT rm.user_id) as online_users,
    r.max_users,
    r.created_at
FROM rooms r
JOIN documents d ON r.doc_id = d.doc_id
LEFT JOIN room_members rm ON r.room_id = rm.room_id AND rm.is_online = 1
WHERE r.is_active = 1
GROUP BY r.room_id;

-- ============================================
-- 触发器 (Triggers)
-- ============================================

-- 自动更新文档的updated_at时间戳
DROP TRIGGER IF EXISTS update_document_timestamp;
CREATE TRIGGER update_document_timestamp
AFTER UPDATE ON documents
BEGIN
    UPDATE documents SET updated_at = CURRENT_TIMESTAMP WHERE doc_id = NEW.doc_id;
END;

-- 自动清理过期会话
DROP TRIGGER IF EXISTS cleanup_expired_sessions;
CREATE TRIGGER cleanup_expired_sessions
AFTER INSERT ON sessions
BEGIN
    DELETE FROM sessions WHERE expires_at < CURRENT_TIMESTAMP;
END;
