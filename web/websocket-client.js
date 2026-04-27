/**
 * 实时协作编辑器 - WebSocket客户端
 */

class CollaborativeClient {
    constructor(serverUrl) {
        this.serverUrl = serverUrl;
        this.ws = null;
        this.isConnected = false;
        this.operationCount = 0;
        this.userId = 'user_' + Math.random().toString(36).substr(2, 9);
        
        this.editor = document.getElementById('editor');
        this.statusDot = document.getElementById('statusDot');
        this.statusText = document.getElementById('statusText');
        this.connectBtn = document.getElementById('connectBtn');
        this.sendBtn = document.getElementById('sendBtn');
        this.logContainer = document.getElementById('logContainer');
        this.opCountElement = document.getElementById('opCount');
        
        // 绑定编辑器事件
        this.editor.addEventListener('input', (e) => this.handleEditorInput(e));
        
        this.addLog('客户端初始化完成');
    }
    
    /**
     * 连接到WebSocket服务器
     */
    connect() {
        if (this.isConnected) {
            this.addLog('已经连接，无需重复连接');
            return;
        }
        
        this.addLog('正在连接到 ' + this.serverUrl + '...');
        
        try {
            this.ws = new WebSocket(this.serverUrl);
            
            this.ws.onopen = () => {
                this.isConnected = true;
                this.updateStatus(true);
                this.addLog('✓ 连接成功！');
                this.connectBtn.textContent = '断开连接';
                this.connectBtn.onclick = () => this.disconnect();
                this.sendBtn.disabled = false;
                
                // 发送加入房间消息
                this.sendMessage({
                    type: 'join',
                    payload: {
                        userId: this.userId,
                        docId: 'doc_demo_001'
                    }
                });
            };
            
            this.ws.onmessage = (event) => {
                this.handleMessage(event.data);
            };
            
            this.ws.onclose = () => {
                this.isConnected = false;
                this.updateStatus(false);
                this.addLog('✗ 连接已关闭');
                this.connectBtn.textContent = '连接服务器';
                this.connectBtn.onclick = () => this.connect();
                this.sendBtn.disabled = true;
            };
            
            this.ws.onerror = (error) => {
                this.addLog('✗ 连接错误: ' + error.message);
                console.error('WebSocket error:', error);
            };
            
        } catch (error) {
            this.addLog('✗ 连接失败: ' + error.message);
        }
    }
    
    /**
     * 断开连接
     */
    disconnect() {
        if (this.ws) {
            this.ws.close();
            this.addLog('主动断开连接');
        }
    }
    
    /**
     * 发送消息
     */
    sendMessage(message) {
        if (!this.isConnected || !this.ws) {
            this.addLog('✗ 未连接，无法发送消息');
            return;
        }
        
        const messageStr = JSON.stringify(message);
        this.ws.send(messageStr);
        this.addLog('→ 发送: ' + message.type);
    }
    
    /**
     * 处理接收到的消息
     */
    handleMessage(data) {
        try {
            const message = JSON.parse(data);
            this.addLog('← 收到: ' + message.type);
            
            switch (message.type) {
                case 'operation':
                    this.applyRemoteOperation(message.payload);
                    break;
                    
                case 'ack':
                    this.addLog('✓ 操作确认: ' + message.payload.opId);
                    break;
                    
                case 'user_joined':
                    this.addUser(message.payload);
                    break;
                    
                case 'user_left':
                    this.removeUser(message.payload);
                    break;
                    
                default:
                    this.addLog('? 未知消息类型: ' + message.type);
            }
            
        } catch (error) {
            this.addLog('✗ 消息解析错误: ' + error.message);
        }
    }
    
    /**
     * 处理编辑器输入
     */
    handleEditorInput(event) {
        if (!this.isConnected) {
            return;
        }
        
        // 简化实现：每次输入都发送整个内容
        // 实际应该使用OT算法生成增量操作
        const operation = {
            opId: 'op_' + Date.now(),
            userId: this.userId,
            version: ++this.operationCount,
            type: 'replace',
            content: this.editor.value
        };
        
        this.sendMessage({
            type: 'operation',
            payload: operation
        });
        
        this.updateOpCount();
    }
    
    /**
     * 应用远程操作
     */
    applyRemoteOperation(operation) {
        this.addLog('应用远程操作: ' + operation.opId);
        
        // 简化实现：直接替换内容
        // 实际应该使用OT算法合并操作
        if (operation.userId !== this.userId) {
            const cursorPos = this.editor.selectionStart;
            this.editor.value = operation.content;
            this.editor.setSelectionRange(cursorPos, cursorPos);
        }
        
        this.updateOpCount();
    }
    
    /**
     * 添加用户到列表
     */
    addUser(user) {
        const userList = document.getElementById('userList');
        const li = document.createElement('li');
        li.className = 'user-item';
        li.id = 'user-' + user.userId;
        li.innerHTML = `
            <div class="user-avatar">${user.username ? user.username[0].toUpperCase() : 'U'}</div>
            <span>${user.username || '匿名用户'}</span>
        `;
        userList.appendChild(li);
        this.addLog('👤 用户加入: ' + (user.username || user.userId));
    }
    
    /**
     * 从列表移除用户
     */
    removeUser(user) {
        const userElement = document.getElementById('user-' + user.userId);
        if (userElement) {
            userElement.remove();
        }
        this.addLog('👋 用户离开: ' + (user.username || user.userId));
    }
    
    /**
     * 更新连接状态显示
     */
    updateStatus(connected) {
        if (connected) {
            this.statusDot.classList.add('connected');
            this.statusText.textContent = '已连接';
        } else {
            this.statusDot.classList.remove('connected');
            this.statusText.textContent = '未连接';
        }
    }
    
    /**
     * 更新操作计数
     */
    updateOpCount() {
        this.opCountElement.textContent = this.operationCount;
    }
    
    /**
     * 添加日志
     */
    addLog(message) {
        const timestamp = new Date().toLocaleTimeString();
        const logEntry = document.createElement('div');
        logEntry.className = 'log-entry';
        logEntry.textContent = `[${timestamp}] ${message}`;
        
        this.logContainer.appendChild(logEntry);
        this.logContainer.scrollTop = this.logContainer.scrollHeight;
        
        // 限制日志数量
        while (this.logContainer.children.length > 50) {
            this.logContainer.removeChild(this.logContainer.firstChild);
        }
    }
}

// ==================== 全局函数 ====================

let client = null;

/**
 * 连接/断开服务器
 */
function connect() {
    if (!client) {
        client = new CollaborativeClient('ws://localhost:8080/ws');
    }
    client.connect();
}

/**
 * 发送测试操作
 */
function sendTestOperation() {
    if (!client || !client.isConnected) {
        alert('请先连接服务器');
        return;
    }
    
    const testContent = '这是一条测试消息 - ' + new Date().toLocaleString();
    client.editor.value = testContent;
    
    const operation = {
        opId: 'op_test_' + Date.now(),
        userId: client.userId,
        version: ++client.operationCount,
        type: 'insert',
        position: 0,
        content: testContent
    };
    
    client.sendMessage({
        type: 'operation',
        payload: operation
    });
    
    client.updateOpCount();
    client.addLog('发送测试操作');
}

/**
 * 清空编辑器
 */
function clearEditor() {
    if (confirm('确定要清空编辑器吗？')) {
        document.getElementById('editor').value = '';
        client.addLog('编辑器已清空');
    }
}

// 页面加载完成后自动初始化
window.addEventListener('load', () => {
    console.log('实时协作编辑器已加载');
});
