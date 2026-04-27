# Web前端Demo - 实时协作编辑器

## 📖 简介

这是一个基于HTML5和WebSocket的简单实时协作编辑器Demo，用于演示RealtimeCollabEngine的核心功能。

## 🚀 快速开始

### 1. 启动后端服务器

```bash
cd /home/hala/RealtimeCollabEngine
./run.sh
```

服务器将在 `ws://localhost:8080/ws` 监听WebSocket连接。

### 2. 打开前端页面

在浏览器中打开：
```
file:///home/hala/RealtimeCollabEngine/web/index.html
```

或者使用简单的HTTP服务器（推荐）：

```bash
cd /home/hala/RealtimeCollabEngine/web
python3 -m http.server 8000
```

然后在浏览器中访问：
```
http://localhost:8000
```

## 💡 使用说明

1. **连接服务器**：点击"连接服务器"按钮建立WebSocket连接
2. **编辑文档**：在文本框中输入内容
3. **实时同步**：多个用户同时打开页面，可以看到彼此的编辑实时同步
4. **查看日志**：右侧面板显示所有操作日志

## 🎨 功能特性

- ✅ 实时WebSocket连接
- ✅ 多用户协作编辑
- ✅ 在线用户列表显示
- ✅ 操作日志记录
- ✅ 美观的渐变UI设计
- ✅ 响应式布局

## 🔧 技术栈

- **前端**: HTML5 + CSS3 + Vanilla JavaScript
- **通信**: WebSocket API
- **样式**: 现代CSS (Flexbox, Gradient, Shadow)

## 📝 注意事项

1. 确保后端服务器已启动
2. 浏览器需要支持WebSocket（所有现代浏览器都支持）
3. 当前实现为简化版本，实际生产环境应使用OT算法处理冲突

## 🐛 故障排除

### 无法连接服务器
- 检查后端是否正在运行
- 确认WebSocket地址是否正确（默认：ws://localhost:8080/ws）
- 检查浏览器控制台是否有错误信息

### 编辑不同步
- 确认多个客户端都已成功连接
- 查看活动日志确认消息发送/接收
- 刷新页面重新连接

---

**版本**: v1.0  
**最后更新**: 2026-04-28
