#!/usr/bin/env python3
"""
RealtimeCollabEngine WebSocket 测试客户端
用于测试WebSocket服务器的功能
"""

import websocket
import json
import time
import uuid
import sys
import threading

class WebSocketTestClient:
    def __init__(self, url="ws://localhost:8080/ws"):
        self.url = url
        self.ws = None
        self.connected = False
        self.messages_received = []
        
    def on_message(self, ws, message):
        """处理接收到的消息"""
        try:
            data = json.loads(message)
            msg_type = data.get('type', 'unknown')
            print(f"\n[RECV] Type: {msg_type}")
            print(f"       Payload: {json.dumps(data.get('payload', {}), indent=2)}")
            
            self.messages_received.append(data)
            
            # 自动响应心跳
            if msg_type == 'heartbeat' and data.get('payload', {}).get('ping'):
                self.send_heartbeat_response()
                
        except json.JSONDecodeError:
            print(f"[RECV] Raw message: {message}")
    
    def on_error(self, ws, error):
        """处理错误"""
        print(f"[ERROR] {error}")
    
    def on_close(self, ws, close_status_code, close_msg):
        """处理连接关闭"""
        self.connected = False
        print(f"[CLOSE] Connection closed (code={close_status_code}, reason={close_msg})")
    
    def on_open(self, ws):
        """处理连接打开"""
        self.connected = True
        print("[OPEN] Connected to WebSocket server")
    
    def connect(self):
        """连接到WebSocket服务器"""
        print(f"Connecting to {self.url}...")
        self.ws = websocket.WebSocketApp(
            self.url,
            on_open=self.on_open,
            on_message=self.on_message,
            on_error=self.on_error,
            on_close=self.on_close
        )
        
        # 在后台线程中运行
        thread = threading.Thread(target=self.ws.run_forever)
        thread.daemon = True
        thread.start()
        
        # 等待连接
        time.sleep(1)
        return self.connected
    
    def send_message(self, message_dict):
        """发送消息"""
        if not self.connected:
            print("[WARN] Not connected")
            return False
        
        message_json = json.dumps(message_dict)
        print(f"\n[SEND] {json.dumps(message_dict, indent=2)}")
        self.ws.send(message_json)
        return True
    
    def send_connect(self, user_id, token):
        """发送连接认证消息"""
        msg = {
            "type": "connect",
            "messageId": str(uuid.uuid4()),
            "timestamp": time.strftime("%Y-%m-%dT%H:%M:%S.000Z", time.gmtime()),
            "payload": {
                "userId": user_id,
                "token": token
            }
        }
        return self.send_message(msg)
    
    def send_join_room(self, room_id):
        """发送加入房间消息"""
        msg = {
            "type": "join_room",
            "messageId": str(uuid.uuid4()),
            "timestamp": time.strftime("%Y-%m-%dT%H:%M:%S.000Z", time.gmtime()),
            "payload": {
                "roomId": room_id
            }
        }
        return self.send_message(msg)
    
    def send_operation(self, op_id, user_id, version, op_type, position, content):
        """发送编辑操作消息"""
        msg = {
            "type": "operation",
            "messageId": str(uuid.uuid4()),
            "timestamp": time.strftime("%Y-%m-%dT%H:%M:%S.000Z", time.gmtime()),
            "payload": {
                "opId": op_id,
                "userId": user_id,
                "version": version,
                "operation": {
                    "type": op_type,
                    "position": position,
                    "content": content
                }
            }
        }
        return self.send_message(msg)
    
    def send_heartbeat(self):
        """发送心跳消息"""
        msg = {
            "type": "heartbeat",
            "messageId": str(uuid.uuid4()),
            "timestamp": time.strftime("%Y-%m-%dT%H:%M:%S.000Z", time.gmtime()),
            "payload": {
                "ping": True
            }
        }
        return self.send_message(msg)
    
    def send_heartbeat_response(self):
        """发送心跳响应"""
        msg = {
            "type": "heartbeat",
            "messageId": str(uuid.uuid4()),
            "timestamp": time.strftime("%Y-%m-%dT%H:%M:%S.000Z", time.gmtime()),
            "payload": {
                "pong": True
            }
        }
        return self.send_message(msg)
    
    def send_leave_room(self, room_id):
        """发送离开房间消息"""
        msg = {
            "type": "leave_room",
            "messageId": str(uuid.uuid4()),
            "timestamp": time.strftime("%Y-%m-%dT%H:%M:%S.000Z", time.gmtime()),
            "payload": {
                "roomId": room_id
            }
        }
        return self.send_message(msg)
    
    def disconnect(self):
        """断开连接"""
        if self.ws:
            self.ws.close()
        self.connected = False

def test_basic_connection():
    """测试基本连接"""
    print("\n" + "="*60)
    print("Test 1: Basic Connection")
    print("="*60)
    
    client = WebSocketTestClient()
    
    if not client.connect():
        print("[FAIL] Failed to connect")
        return False
    
    # 发送连接认证
    client.send_connect("user_001", "test-token-123456789")
    time.sleep(0.5)
    
    # 发送心跳
    client.send_heartbeat()
    time.sleep(0.5)
    
    print("[PASS] Basic connection test passed")
    client.disconnect()
    return True

def test_room_operations():
    """测试房间操作"""
    print("\n" + "="*60)
    print("Test 2: Room Operations")
    print("="*60)
    
    client = WebSocketTestClient()
    
    if not client.connect():
        print("[FAIL] Failed to connect")
        return False
    
    # 认证
    client.send_connect("user_002", "test-token-123456789")
    time.sleep(0.5)
    
    # 加入房间
    client.send_join_room("room_test_001")
    time.sleep(0.5)
    
    # 发送编辑操作
    client.send_operation(
        op_id="op_001",
        user_id="user_002",
        version=1,
        op_type="insert",
        position=0,
        content="Hello World"
    )
    time.sleep(0.5)
    
    # 离开房间
    client.send_leave_room("room_test_001")
    time.sleep(0.5)
    
    print("[PASS] Room operations test passed")
    client.disconnect()
    return True

def main():
    """主函数"""
    print("\n" + "="*60)
    print("  RealtimeCollabEngine WebSocket Test Client")
    print("="*60)
    
    # 检查是否安装了websocket-client
    try:
        import websocket
    except ImportError:
        print("\n[ERROR] websocket-client library not installed")
        print("Install it with: pip install websocket-client")
        sys.exit(1)
    
    # 运行测试
    tests = [
        test_basic_connection,
        test_room_operations,
    ]
    
    passed = 0
    failed = 0
    
    for test in tests:
        try:
            if test():
                passed += 1
            else:
                failed += 1
        except Exception as e:
            print(f"[FAIL] Test failed with exception: {e}")
            failed += 1
    
    # 显示测试结果
    print("\n" + "="*60)
    print(f"Test Results: {passed} passed, {failed} failed")
    print("="*60 + "\n")
    
    return 0 if failed == 0 else 1

if __name__ == "__main__":
    sys.exit(main())
